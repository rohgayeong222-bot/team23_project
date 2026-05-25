#include "common.h"
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

volatile sig_atomic_t keep_running = 1;

metrics_t global_metrics;
incident_t global_incident;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; 

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("\n[Tracker] SIGINT 감지. 리소스 해제 및 안전 종료를 진행합니다.\n");
        keep_running = 0;
    }
}

int read_line(int fd, char *buffer, int max_len) {
    int i = 0; char c;
    while (i < max_len - 1) {
        ssize_t bytes_read = read(fd, &c, 1);
        if (bytes_read == 0) return i;
        if (bytes_read < 0) return -1;
        buffer[i++] = c;
        if (c == '\n') break;
    }
    buffer[i] = '\0';
    return i;
}

// 현재 날짜를 YYYY-MM-DD 형식 문자열로 반환
void get_current_date(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(buffer, "%04d-%02d-%02d", 
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

// 30일이 지난 오래된 리포트 폴더를 자동으로 삭제하여 디스크 용량 관리
void enforce_retention_policy() {
    printf("[System] 데이터 보존 정책 확인 중 (30일 초과 로그 정리)...\n");
    // find 시스템 콜을 활용한 로그 로테이션(Log Rotation) 구현
    system("find reports/* -type d -mtime +30 -exec rm -rf {} + 2>/dev/null");
}

int main() {
    signal(SIGINT, sig_handler);
    analysis_init(&global_metrics, &global_incident);

    // --- [스레드 2: 소켓 서버 (네트워크 통신 처리)] ---
    pthread_t socket_thread;
    if (pthread_create(&socket_thread, NULL, socket_server_thread, NULL) != 0) {
        perror("[Error] 네트워크 스레드 생성 실패");
        return 1;
    }
    pthread_detach(socket_thread);

    // --- [스레드 1: 메인 로직 (Nginx 로그 파일 실시간 감시)] ---
    off_t last_offset = 0;
    struct stat file_stat;
    const char* log_file_path = "/var/log/nginx/error.log";

    printf("[Tracker] Nginx 에러 로그 파일(/var/log/nginx/error.log) 모니터링 시작.\n");
    
    // 시스템 시작 시 오래된 로그 파일 정리 수행
    enforce_retention_policy();

    while (keep_running) {
        if (stat(log_file_path, &file_stat) == 0) {
            if (file_stat.st_size > last_offset) {
                int fd = open(log_file_path, O_RDONLY);
                if (fd >= 0) {
                    lseek(fd, last_offset, SEEK_SET);

                    char line[512];
                    while (read_line(fd, line, sizeof(line)) > 0) {
                        if (line[strlen(line) - 1] != '\n') continue;

                        log_entry_t entry;
                        if (parser_parse_line(line, &entry)) {
                            // 공유 데이터 보호를 위한 상호배제(Mutex)
                            pthread_mutex_lock(&data_mutex);
                            analysis_update(&global_metrics, &global_incident, &entry);
                            pthread_mutex_unlock(&data_mutex);
                        }
                    }
                    last_offset = lseek(fd, 0, SEEK_CUR);
                    close(fd);

                    // 리포트 디렉토리 파티셔닝 및 파일 저장
                    pthread_mutex_lock(&data_mutex);
                    
                    mkdir("reports", 0755); 
                    
                    char date_str[32];
                    get_current_date(date_str); 
                    
                    char date_folder_path[256];
                    sprintf(date_folder_path, "reports/%s", date_str);
                    mkdir(date_folder_path, 0755); 
                    
                    char summary_path[256], incident_path[256];
                    sprintf(summary_path, "%s/summary.txt", date_folder_path);
                    sprintf(incident_path, "%s/incident.txt", date_folder_path);
                    
                    report_write_summary(summary_path, &global_metrics);
                    report_write_incident(incident_path, &global_incident);
                    
                    pthread_mutex_unlock(&data_mutex);
                }
            }
        }
        usleep(500000); // 0.5초 대기 (CPU 점유율 최적화)
    }

    // --- [안전 종료 처리 (Graceful Shutdown)] ---
    pthread_mutex_lock(&data_mutex);
    mkdir("reports", 0755);
    char final_date_str[32];
    get_current_date(final_date_str);
    char final_date_folder_path[256];
    sprintf(final_date_folder_path, "reports/%s", final_date_str);
    mkdir(final_date_folder_path, 0755);
    
    char final_summary_path[256], final_incident_path[256];
    sprintf(final_summary_path, "%s/summary.txt", final_date_folder_path);
    sprintf(final_incident_path, "%s/incident.txt", final_date_folder_path);
    
    report_write_summary(final_summary_path, &global_metrics);
    report_write_incident(final_incident_path, &global_incident);
    pthread_mutex_unlock(&data_mutex);

    printf("[Tracker] 파일 동기화 완료. 시스템을 종료합니다.\n");
    return 0;
}
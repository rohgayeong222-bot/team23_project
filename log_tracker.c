#include "common.h"
#include <time.h>
#include <sys/stat.h>

volatile sig_atomic_t keep_running = 1;

metrics_t global_metrics;
incident_t global_incident;
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; 

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("\n[Tracker] 강제 종료 감지! 최종 저장 후 종료합니다...\n");
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

// 현재 날짜를 YYYY-MM-DD 형식 문자열로 만들어주는 함수
void get_current_date(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(buffer, "%04d-%02d-%02d", 
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

int main() {
    signal(SIGINT, sig_handler);
    analysis_init(&global_metrics, &global_incident);

    pthread_t socket_thread;
    pthread_create(&socket_thread, NULL, socket_server_thread, NULL);

    off_t last_offset = 0;
    struct stat file_stat;
    
    // Nginx 실제 에러 로그 경로
    const char* log_file_path = "/var/log/nginx/error.log";

    printf("[Tracker] Nginx 실제 웹서버와 연동되었습니다! 감시 시작...\n");

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
                            pthread_mutex_lock(&data_mutex);
                            analysis_update(&global_metrics, &global_incident, &entry);
                            pthread_mutex_unlock(&data_mutex);
                        }
                    }
                    last_offset = lseek(fd, 0, SEEK_CUR);
                    close(fd);

                    // 실시간 리포트 저장 (폴더 > 날짜 > 파일)
                    pthread_mutex_lock(&data_mutex);
                    
                    mkdir("reports", 0755); // 1. 최상위 폴더 생성
                    
                    char date_str[32];
                    get_current_date(date_str); // 2. 오늘 날짜 가져오기
                    
                    char date_folder_path[256];
                    sprintf(date_folder_path, "reports/%s", date_str);
                    mkdir(date_folder_path, 0755); // 3. 날짜별 하위 폴더 생성
                    
                    char summary_path[256], incident_path[256];
                    sprintf(summary_path, "%s/summary.txt", date_folder_path);
                    sprintf(incident_path, "%s/incident.txt", date_folder_path);
                    
                    report_write_summary(summary_path, &global_metrics);
                    report_write_incident(incident_path, &global_incident);
                    
                    pthread_mutex_unlock(&data_mutex);
                }
            }
        }
        usleep(500000); 
    }

    //종료 시 최종 저장 (폴더 > 날짜 > 파일)]
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

    return 0;
}
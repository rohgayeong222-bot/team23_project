#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// 시스템 상태 정의
typedef enum {
    HEALTHY = 0,
    WARNING = 1,
    CRITICAL = 2
} status_t;

// 로그 엔트리 구조체 (Nginx 에러 로그 파싱 결과용)
typedef struct {
    char date[32];
    char time[32];
    char level[16];
    char message[256];
} log_entry_t;

// 통계 지표 구조체
typedef struct {
    int total_requests;
    int error_count;
    double error_ratio;
    status_t current_status;
} metrics_t;

// 사건(Incident) 기록 구조체
typedef struct {
    char first_critical_time[64];
    char last_error_msg[256];
    int is_active;
} incident_t;

// 멀티스레드 동기화를 위한 공유 자원 및 뮤텍스 선언 (외부 모듈 참조 가능)
extern metrics_t global_metrics;
extern incident_t global_incident;
extern pthread_mutex_t data_mutex;
extern volatile sig_atomic_t keep_running;

// 각 모듈별 핵심 함수 프로토타입 선언
int parser_parse_line(const char *line, log_entry_t *entry);
void analysis_init(metrics_t *metrics, incident_t *incident);
void analysis_update(metrics_t *metrics, incident_t *incident, const log_entry_t *entry);
void report_write_summary(const char *path, const metrics_t *metrics);
void report_write_incident(const char *path, const incident_t *incident);
void *socket_server_thread(void *arg);

#endif
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// 서비스 상태 상수 정의 (정상, 경고, 위험)
typedef enum {
    STATE_HEALTHY = 0,
    STATE_WARNING = 1,
    STATE_CRITICAL = 2
} service_state_t;

// 단일 로그 데이터를 담는 구조체
typedef struct {
    char timestamp[64];
    char level[16];
    char message[256];
} log_entry_t;

// 전체 시스템 통계 지표 구조체
typedef struct {
    int total_logs;
    int error_logs;
    int notice_logs;
    int warn_logs;
    double error_ratio;
    service_state_t state;
} metrics_t;

// 이상 징후(Incident) 정보를 담는 구조체
typedef struct {
    int active;
    char start_time[64];
    char end_time[64];
    int log_count_in_incident;
    int error_count_in_incident;
    char first_error_message[256];
} incident_t;

// 전역 변수 및 데이터 보호용 뮤텍스 선언
extern metrics_t global_metrics;
extern incident_t global_incident;
extern pthread_mutex_t data_mutex;

// 모듈별 핵심 함수 선언
int parser_parse_line(const char *line, log_entry_t *entry);
void analysis_init(metrics_t *metrics, incident_t *incident);
void analysis_update(metrics_t *metrics, incident_t *incident, const log_entry_t *entry);
void report_write_summary(const char *filename, const metrics_t *metrics);
void report_write_incident(const char *filename, const incident_t *incident);
void* socket_server_thread(void* arg);

#endif
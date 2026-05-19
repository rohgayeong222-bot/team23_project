#include "common.h"

// 현재 통계 지표를 요약 리포트(.txt) 파일로 기록하는 함수
void report_write_summary(const char *filename, const metrics_t *metrics) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    char buffer[512];
    sprintf(buffer, "=== [Summary Report] ===\nTotal Logs: %d\nErrors: %d\nError Ratio: %.2f%%\nState: %d (0:H, 1:W, 2:C)\n",
            metrics->total_logs, metrics->error_logs, metrics->error_ratio, metrics->state);

    write(fd, buffer, strlen(buffer));
    close(fd);
}

// 위험 상태 발생 시 인시던트 리포트(.txt) 파일을 기록하는 함수
void report_write_incident(const char *filename, const incident_t *incident) {
    if (incident->active == 0) return;

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    char buffer[512];
    sprintf(buffer, "!!! [INCIDENT DETECTED] !!!\nStart Time: %s\nFirst Error: %s\nLogs during incident: %d\nErrors during incident: %d\n",
            incident->start_time, incident->first_error_message, 
            incident->log_count_in_incident, incident->error_count_in_incident);

    write(fd, buffer, strlen(buffer));
    close(fd);
}
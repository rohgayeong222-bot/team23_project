#include "common.h"

// 저수준 시스템 콜(open, write, close)을 사용한 요약 리포트 저장
void report_write_summary(const char *path, const metrics_t *metrics) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    char buffer[512];
    const char *status_str[] = {"HEALTHY", "WARNING", "CRITICAL"};
    
    int len = sprintf(buffer, 
        "=======================================\n"
        "        SYSTEM SUMMARY REPORT          \n"
        "=======================================\n"
        "Total Requests Analyzed : %d\n"
        "Total Errors Detected   : %d\n"
        "Current Error Ratio     : %.2f%%\n"
        "Current System Status   : %s\n"
        "=======================================\n",
        metrics->total_requests, metrics->error_count, 
        metrics->error_ratio, status_str[metrics->current_status]);

    write(fd, buffer, len);
    close(fd);
}

// 저수준 시스템 콜을 사용한 인시던트(비상) 보고서 저장
void report_write_incident(const char *path, const incident_t *incident) {
    // CRITICAL 상태가 아니라서 활성화되지 않은 경우 리포트를 작성하지 않음
    if (!incident->is_active) return;

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    char buffer[512];
    int len = sprintf(buffer,
        "=======================================\n"
        "       🚨 SYSTEM INCIDENT REPORT 🚨    \n"
        "=======================================\n"
        "Incident Triggered At  : %s\n"
        "Last Critical Error    : %s\n"
        "Action Required        : Please Check Nginx Infrastructure!\n"
        "=======================================\n",
        incident->first_critical_time, incident->last_error_msg);

    write(fd, buffer, len);
    close(fd);
}
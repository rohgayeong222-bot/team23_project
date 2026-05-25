#include "common.h"

void analysis_init(metrics_t *metrics, incident_t *incident) {
    metrics->total_requests = 0;
    metrics->error_count = 0;
    metrics->error_ratio = 0.0;
    metrics->current_status = HEALTHY;

    incident->is_active = 0;
    memset(incident->first_critical_time, 0, sizeof(incident->first_critical_time));
    memset(incident->last_error_msg, 0, sizeof(incident->last_error_msg));
}

void analysis_update(metrics_t *metrics, incident_t *incident, const log_entry_t *entry) {
    metrics->total_requests++;
    metrics->error_count++; // 파서가 error 레벨만 걸러서 주므로 무조건 증가
    
    // 에러 비율 동적 계산
    metrics->error_ratio = ((double)metrics->error_count / metrics->total_requests) * 100.0;

    // 시스템 상태 결정 상태머신 (임계치: 30%)
    status_t old_status = metrics->current_status;
    
    if (metrics->error_ratio >= 30.0) {
        metrics->current_status = CRITICAL;
    } else if (metrics->error_ratio >= 10.0) {
        metrics->current_status = WARNING;
    } else {
        metrics->current_status = HEALTHY;
    }

    // HEALTHY/WARNING 상태에서 최초로 CRITICAL 상태로 진입 시 사건 대장 생성
    if (metrics->current_status == CRITICAL && old_status != CRITICAL) {
        incident->is_active = 1;
        sprintf(incident->first_critical_time, "%s %s", entry->date, entry->time);
    }

    // 가장 최근에 발생한 위험 에러 메시지 상시 동기화
    strncpy(incident->last_error_msg, entry->message, sizeof(incident->last_error_msg) - 1);
}
#include "common.h"

// 통계 및 인시던트 구조체를 초기화하는 함수
void analysis_init(metrics_t *metrics, incident_t *incident) {
    memset(metrics, 0, sizeof(metrics_t));
    memset(incident, 0, sizeof(incident_t));
    metrics->state = STATE_HEALTHY;
}

// 새 로그 데이터를 바탕으로 통계 갱신 및 상태(정상/경고/위험)를 판정하는 함수
void analysis_update(metrics_t *metrics, incident_t *incident, const log_entry_t *entry) {
    metrics->total_logs++;

    if (strstr(entry->level, "error") != NULL) metrics->error_logs++;
    else if (strstr(entry->level, "notice") != NULL) metrics->notice_logs++;
    else if (strstr(entry->level, "warn") != NULL) metrics->warn_logs++;

    if (metrics->total_logs > 0) {
        metrics->error_ratio = ((double)metrics->error_logs / metrics->total_logs) * 100.0;
    }

    if (metrics->error_ratio >= 30.0) {
        metrics->state = STATE_CRITICAL;
        if (incident->active == 0) {
            incident->active = 1;
            strcpy(incident->start_time, entry->timestamp);
            strcpy(incident->first_error_message, entry->message);
        }
    } else if (metrics->error_ratio >= 10.0) {
        metrics->state = STATE_WARNING;
    } else {
        metrics->state = STATE_HEALTHY;
    }

    if (incident->active == 1) {
        incident->log_count_in_incident++;
        if (strstr(entry->level, "error") != NULL) {
            incident->error_count_in_incident++;
        }
    }
}
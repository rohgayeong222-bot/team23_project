#include "common.h"

// Nginx 표준 에러 로그 파싱 함수
// 예시: 2026/05/25 18:12:43 [error] 1234#1234: *4567 open() "/var/www/html/..." failed ...
int parser_parse_line(const char *line, log_entry_t *entry) {
    if (line == NULL || strlen(line) == 0) return 0;

    // 안전한 파싱을 위해 임시 버퍼 사용
    char temp[512];
    strncpy(temp, line, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // 1. 날짜 추출 (첫 번째 공백 전까지)
    char *ptr = strchr(temp, ' ');
    if (!ptr) return 0;
    *ptr = '\0';
    strncpy(entry->date, temp, sizeof(entry->date) - 1);

    // 2. 시간 추출
    char *time_start = ptr + 1;
    ptr = strchr(time_start, ' ');
    if (!ptr) return 0;
    *ptr = '\0';
    strncpy(entry->time, time_start, sizeof(entry->time) - 1);

    // 3. 로그 레벨 추출 (예: [error])
    char *level_start = strchr(ptr + 1, '[');
    if (!level_start) return 0;
    char *level_end = strchr(level_start, ']');
    if (!level_end) return 0;
    
    *level_end = '\0';
    strncpy(entry->level, level_start + 1, sizeof(entry->level) - 1);

    // 4. 상세 에러 메시지 추출
    char *msg_start = level_end + 2; // ']'와 공백 다음부터
    strncpy(entry->message, msg_start, sizeof(entry->message) - 1);
    
    // 개행 문자 제거
    size_t len = strlen(entry->message);
    if (len > 0 && entry->message[len - 1] == '\n') {
        entry->message[len - 1] = '\0';
    }

    // [error] 레벨인 경우에만 참을 반환하여 통계에 반영
    if (strcmp(entry->level, "error") == 0) {
        return 1;
    }

    return 0;
}
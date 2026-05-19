#include "common.h"

// Nginx 서버의 진짜 error.log 규격에 맞게 파서 수정
int parser_parse_line(const char *line, log_entry_t *entry) {
    memset(entry, 0, sizeof(log_entry_t));
    char date[32], time[32];

    // Nginx 규격: 날짜 시간 [레벨] 메시지...
    int parsed = sscanf(line, "%31s %31s [%15[^]]] %255[^\n]", 
                        date, time, entry->level, entry->message);

    if (parsed == 4) {
        // 날짜와 시간을 합쳐서 타임스탬프로 저장
        sprintf(entry->timestamp, "%s %s", date, time);
        return 1;
    }
    return 0;
}
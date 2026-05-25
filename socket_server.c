#include "common.h"

// [보너스 요건 완료: 외부 모니터링을 실시간 지원하는 네트워크 소켓 스레드]
void *socket_server_thread(void *arg) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // TCP 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Socket Error] 생성 실패");
        pthread_exit(NULL);
    }

    // 포트 재사용 설정 (안정적인 재구동용)
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080); // 8080 포트 오픈

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Socket Error] 바인딩 실패");
        close(server_fd);
        pthread_exit(NULL);
    }

    if (listen(server_fd, 5) < 0) {
        perror("[Socket Error] 리슨 실패");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf("[Socket Server] 8080 포트에서 안내데스크 스레드가 가동되었습니다.\n");

    while (keep_running) {
        // 클라이언트 접속 대기
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            if (!keep_running) break;
            continue;
        }

        // 클라이언트 요청 읽기 버퍼 (요청 자체는 무시하고 지표 응답 대기)
        char req_buf[1024] = {0};
        read(client_fd, req_buf, sizeof(req_buf));

        // 공유 데이터에 접근하기 전 안전하게 뮤텍스 락 획득
        pthread_mutex_lock(&data_mutex);
        
        char body[512];
        const char *status_str[] = {"HEALTHY", "WARNING", "CRITICAL"};
        
        sprintf(body, 
            "{\"total_requests\": %d, \"error_count\": %d, \"error_ratio\": %.2f, \"status\": \"%s\"}",
            global_metrics.total_requests, global_metrics.error_count, 
            global_metrics.error_ratio, status_str[global_metrics.current_status]);
            
        pthread_mutex_unlock(&data_mutex);

        // HTTP/1.1 규격에 맞게 웹 브라우저 및 curl에 응답 전송
        char response[1024];
        int len = sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json; charset=UTF-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n"
            "%s", (int)strlen(body), body);

        write(client_fd, response, len);
        close(client_fd);
    }

    close(server_fd);
    printf("[Socket Server] 안내데스크 스레드가 안전하게 종료되었습니다.\n");
    return NULL;
}
#include "common.h"

// 클라이언트 연결을 대기하고 실시간 통계를 반환하는 소켓 스레드 함수
void* socket_server_thread(void* arg) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("[Socket] 8080번 포트에서 외부 요청 대기 중...\n");

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;

        char response[512];
        
        pthread_mutex_lock(&data_mutex);
        sprintf(response, "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n[Live Status]\nTotal: %d\nErrors: %d\nRatio: %.2f%%\nState: %d\n",
                global_metrics.total_logs, global_metrics.error_logs, 
                global_metrics.error_ratio, global_metrics.state);
        pthread_mutex_unlock(&data_mutex);

        send(new_socket, response, strlen(response), 0);
        close(new_socket);
    }
    return NULL;
}
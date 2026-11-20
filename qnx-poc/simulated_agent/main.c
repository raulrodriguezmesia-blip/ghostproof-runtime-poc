/*
 * main.c - Simulated Agent: genera acciones y las envía al Policy Engine.
 *
 * Simula distintos agentes (safety, datos, potencia) enviando acciones
 * para ejercitar las reglas de política del engine.
 *
 * Build: gcc -o simulated_agent main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#define SOCK int
#define CLOSE_SOCK closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCK int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCK close
#endif

#define POLICY_PORT 9090
#define BUF_SIZE 4096

typedef struct {
    const char *actor;
    const char *action;
    const char *resource;
} scenario_t;

static const scenario_t SCENARIOS[] = {
    {"agent-safety-01", "brake_apply", "brake_front"},     /* POL-SAFETY-01 -> ALLOW */
    {"agent-telemetry", "brake_apply", "brake_front"},     /* POL-SAFETY-02 -> DENY */
    {"agent-safety-02", "brake_release", "brake_rear"},    /* POL-SAFETY-01 -> ALLOW */
    {"agent-telem-02",   "read",      "ecu_engine"},        /* POL-DATA-01 -> ALLOW */
    {"agent-telem-03",   "write",     "ecu_engine"},       /* POL-DATA-03 -> DENY */
    {"agent-safety-03",  "write",     "ecu_engine"},       /* POL-DATA-02 -> ALLOW */
    {"agent-power-01",   "set",       "powertrain_torque"},/* POL-POWER-01 -> ALLOW */
    {"agent-maint",      "set",       "powertrain_torque"},/* POL-POWER-02 -> DENY */
    {"unknown-agent",    "unknown",   "unknown_resource"}, /* POL-DEFAULT-DENY */
};

static int send_recv(const char *actor, const char *action, const char *resource, char *out, size_t outsz)
{
    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(POLICY_PORT);
    srv.sin_addr.s_addr = inet_addr("127.0.0.1");

    SOCK s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return -1;
    if (connect(s, (struct sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) { CLOSE_SOCK(s); return -1; }

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg),
        "{\"actor_id\":\"%s\",\"action_type\":\"%s\",\"resource_id\":\"%s\"}",
        actor, action, resource);

    send(s, msg, strlen(msg), 0);

    /* shutdown write so server sees EOF */
#if defined(_WIN32)
    shutdown(s, SD_SEND);
#else
    shutdown(s, SHUT_WR);
#endif

    int n = recv(s, out, outsz - 1, 0);
    CLOSE_SOCK(s);
    if (n <= 0) return -1;
    out[n] = '\0';
    return 0;
}

int main(void)
{
#if defined(_WIN32)
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    int n = sizeof(SCENARIOS) / sizeof(SCENARIOS[0]);
    printf("Simulated Agent: enviando %d escenarios al Policy Engine...\n", n);

    for (int i = 0; i < n; i++) {
        char reply[BUF_SIZE];
        if (send_recv(SCENARIOS[i].actor, SCENARIOS[i].action, SCENARIOS[i].resource, reply, sizeof(reply)) < 0) {
            printf("[%02d] ERROR: no se pudo conectar al Policy Engine\n", i+1);
            continue;
        }
        printf("[%02d] %-22s %-16s %-20s -> %s",
               i+1, SCENARIOS[i].actor, SCENARIOS[i].action,
               SCENARIOS[i].resource, reply);
    }

#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}
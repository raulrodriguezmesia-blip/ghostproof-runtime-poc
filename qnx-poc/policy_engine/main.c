/*
 * main.c - Policy Engine: servicio que recibe acciones y evalúa políticas.
 *
 * Escucha conexiones TCP, evalúa la política contra la tabla de reglas
 * y envía la decisión (ALLOW/DENY) más el registro de evidencia al
 * Evidence Store.
 *
 * Build: gcc -o policy_engine main.c policy_eval.c -lm
 */
#include "policy_rules.h"

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

#define LISTEN_PORT 9090
#define BUF_SIZE 4096

/* Prototipo de policy_evaluate (policy_eval.c) */
int policy_evaluate(const char *actor, const char *action, const char *resource,
                    const char *context_json, char *matched_policy_id,
                    size_t matched_policy_id_size, int64_t *latency_ns);

/* Cliente TCP: envía la evidencia al Evidence Store */
static void send_evidence(const char *actor, const char *action, const char *resource,
                          const char *policy_id, const char *decision, int64_t latency_ns)
{
    struct sockaddr_in ev_addr;
    memset(&ev_addr, 0, sizeof(ev_addr));
    ev_addr.sin_family = AF_INET;
    ev_addr.sin_port = htons(9091);  /* Evidence Store */
    ev_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    SOCK es = socket(AF_INET, SOCK_STREAM, 0);
    if (es == INVALID_SOCKET) return;
    if (connect(es, (struct sockaddr*)&ev_addr, sizeof(ev_addr)) == SOCKET_ERROR) {
        CLOSE_SOCK(es); return;
    }

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg),
        "{\"actor_id\":\"%s\",\"action_type\":\"%s\",\"resource_id\":\"%s\","
        "\"policy_id\":\"%s\",\"decision\":\"%s\",\"latency_ns\":%lld}",
        actor, action, resource, policy_id, decision, (long long)latency_ns);

    send(es, msg, strlen(msg), 0);
    CLOSE_SOCK(es);
}

static void handle_client(SOCK client_sock)
{
    char buf[BUF_SIZE];
    int n = recv(client_sock, buf, BUF_SIZE - 1, 0);
    if (n <= 0) { CLOSE_SOCK(client_sock); return; }
    buf[n] = '\0';

    /* Parse JSON simple: {"actor_id":"...","action_type":"...","resource_id":"...","context_json":"..."} */
    char actor[64] = "unknown";
    char action[64] = "unknown";
    char resource[64] = "unknown";
    char context_json[512] = "";

    const char *p = buf;
    char key[64], val[512];
    while (*p) {
        while (*p && *p != '"') p++;
        if (!*p) break;
        p++;
        int ki = 0;
        while (*p && *p != '"' && ki < 63) key[ki++] = *p++;
        key[ki] = '\0';
        if (*p == '"') p++;
        while (*p && *p != ':') p++;
        if (*p == ':') p++;
        int quoted = (*p == '"'); if (quoted) p++;
        int vi = 0;
        while (*p && !(quoted ? *p == '"' : (*p == ',' || *p == '}')) && vi < 511) val[vi++] = *p++;
        val[vi] = '\0';
        if (quoted && *p == '"') p++;

        if (strcmp(key, "actor_id")==0)     strncpy(actor, val, 63);
        else if (strcmp(key, "action_type")==0) strncpy(action, val, 63);
        else if (strcmp(key, "resource_id")==0) strncpy(resource, val, 63);
        else if (strcmp(key, "context_json")==0) strncpy(context_json, val, 511);

        if (*p == ',') p++;
        if (*p == '}') break;
    }

    char matched_policy[32];
    int64_t latency_ns = 0;

    int decision = policy_evaluate(actor, action, resource, context_json,
                                   matched_policy, sizeof(matched_policy), &latency_ns);

    const char *decision_str = (decision == 1) ? "ALLOW" : "DENY";

    /* Enviar respuesta al agente */
    char reply[BUF_SIZE];
    snprintf(reply, sizeof(reply),
        "{\"decision\":\"%s\",\"policy_id\":\"%s\",\"latency_ns\":%lld}\n",
        decision_str, matched_policy, (long long)latency_ns);
    send(client_sock, reply, strlen(reply), 0);

    /* Enviar al Evidence Store */
    send_evidence(actor, action, resource, matched_policy, decision_str, latency_ns);

    CLOSE_SOCK(client_sock);
}

int main(void)
{
#if defined(_WIN32)
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    SOCK listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        perror("bind"); return 1;
    }
    if (listen(listen_sock, 16) == SOCKET_ERROR) { perror("listen"); return 1; }

    fprintf(stderr, "[policy_engine] escuchando en puerto %d\n", LISTEN_PORT);

    while (1) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        SOCK client_sock = accept(listen_sock, (struct sockaddr*)&caddr, &clen);
        if (client_sock == INVALID_SOCKET) continue;
        handle_client(client_sock);
    }

    CLOSE_SOCK(listen_sock);
#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}
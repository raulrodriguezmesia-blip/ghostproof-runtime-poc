/*
 * main.c - Evidence Store: servicio de persistencia de decisiones de política.
 *
 * Escucha conexiones TCP (localhost) y recibe registros de evidencia como
 * líneas de texto JSON. Los persiste en SQLite y responde ACK.
 *
 * Build: gcc -o evidence_store main.c evidence_store.c -lsqlite3
 */
#include "evidence_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define SOCK int
#define CLOSE_SOCK closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define SOCK int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCK close
#endif

#define LISTEN_PORT 9091
#define BUF_SIZE 4096
#define DB_PATH "evidence.db"

static void handle_client(SOCK client_sock)
{
    char buf[BUF_SIZE];
    int n = recv(client_sock, buf, BUF_SIZE - 1, 0);
    if (n <= 0) { CLOSE_SOCK(client_sock); return; }
    buf[n] = '\0';

    /* Esperamos JSON simple:
       {"actor":"...","action":"...","resource":"...","policy_id":"...","decision":"ALLOW|DENY","latency_ns":123,"context_json":"..."}
    */
    evidence_record_t rec;
    memset(&rec, 0, sizeof(rec));
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        rec.timestamp_ns = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    }

    /* Parse manual: buscar claves */
    const char *p = buf;
    char key[64], val[512];
    int have_actor=0, have_action=0, have_resource=0, have_policy=0, have_decision=0, have_latency=0;

    while (*p) {
        /* saltar hasta '"' */
        while (*p && *p != '"') p++;
        if (!*p) break;
        p++; /* skip '" */
        int ki = 0;
        while (*p && *p != '"' && ki < (int)sizeof(key)-1) key[ki++] = *p++;
        key[ki] = '\0';
        if (*p == '"') p++;
        /* saltar ':' */
        while (*p && *p != ':') p++;
        if (*p == ':') p++;
        /* saltar '" si es string */
        int quoted = (*p == '"'); if (quoted) p++;
        int vi = 0;
        while (*p && !(quoted ? *p == '"' : (*p == ',' || *p == '}')) && vi < (int)sizeof(val)-1) val[vi++] = *p++;
        val[vi] = '\0';
        if (quoted && *p == '"') p++;

        if (strcmp(key, "actor_id")==0)     { strncpy(rec.actor_id, val, EV_ACTOR_LEN-1); have_actor=1; }
        else if (strcmp(key, "action_type")==0) { strncpy(rec.action_type, val, EV_ACTION_LEN-1); have_action=1; }
        else if (strcmp(key, "resource_id")==0) { strncpy(rec.resource_id, val, EV_RESOURCE_LEN-1); have_resource=1; }
        else if (strcmp(key, "policy_id")==0)   { strncpy(rec.policy_id, val, EV_POLICY_LEN-1); have_policy=1; }
        else if (strcmp(key, "decision")==0)    { strncpy(rec.decision, val, EV_DECISION_LEN-1); have_decision=1; }
        else if (strcmp(key, "latency_ns")==0)  { rec.latency_ns = atoll(val); have_latency=1; }
        else if (strcmp(key, "context_json")==0) { strncpy(rec.context_json, val, EV_CONTEXT_LEN-1); }

        if (*p == ',') p++;
        if (*p == '}') break;
    }

    if (!have_actor)  strncpy(rec.actor_id, "unknown", EV_ACTOR_LEN-1);
    if (!have_action) strncpy(rec.action_type, "unknown", EV_ACTION_LEN-1);
    if (!have_resource) strncpy(rec.resource_id, "unknown", EV_RESOURCE_LEN-1);
    if (!have_policy) strncpy(rec.policy_id, "POL-UNKNOWN", EV_POLICY_LEN-1);
    if (!have_decision) strncpy(rec.decision, "DENY", EV_DECISION_LEN-1);
    if (!have_latency) rec.latency_ns = 0;

    int rc = evidence_insert(&rec);
    const char *reply = (rc == 0) ? "ACK\n" : "NACK\n";
    send(client_sock, reply, strlen(reply), 0);
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
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        perror("bind"); return 1;
    }
    if (listen(listen_sock, 16) == SOCKET_ERROR) { perror("listen"); return 1; }

    if (evidence_init(DB_PATH) != 0) {
        fprintf(stderr, "evidence_init failed\n");
        return 1;
    }

    fprintf(stderr, "[evidence_store] escuchando en puerto %d, db=%s\n", LISTEN_PORT, DB_PATH);

    while (1) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        SOCK client_sock = accept(listen_sock, (struct sockaddr*)&caddr, &clen);
        if (client_sock == INVALID_SOCKET) continue;
        handle_client(client_sock);
    }

    evidence_close();
    CLOSE_SOCK(listen_sock);
#if defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}
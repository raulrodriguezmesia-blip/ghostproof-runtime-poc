/*
 * evidence_store.h - GhostProof Evidence Store API.
 *
 * Thread-safe SQLite persistence for policy decision evidence.
 * Build: gcc -o evidence_store evidence_store.c -lsqlite3
 */
#ifndef EVIDENCE_STORE_H
#define EVIDENCE_STORE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EV_ACTOR_LEN       64
#define EV_ACTION_LEN      64
#define EV_RESOURCE_LEN    64
#define EV_POLICY_LEN      32
#define EV_DECISION_LEN    8
#define EV_CONTEXT_LEN     512

typedef struct {
    int64_t  timestamp_ns;
    char     actor_id[EV_ACTOR_LEN];
    char     action_type[EV_ACTION_LEN];
    char     resource_id[EV_RESOURCE_LEN];
    char     policy_id[EV_POLICY_LEN];
    char     decision[EV_DECISION_LEN];
    int64_t  latency_ns;
    char     context_json[EV_CONTEXT_LEN];
} evidence_record_t;

/* Open / initialize the evidence store (creates tables if needed). */
int  evidence_init(const char *db_path);

/* Insert a single evidence record. Returns 0 on success, non-zero on error. */
int  evidence_insert(const evidence_record_t *rec);

/* Query helpers. */
int  evidence_query_by_actor(const char *actor_id,
                             evidence_record_t *out,
                             size_t max_out,
                             size_t *count);

int  evidence_query_by_timestamp(int64_t ts_from,
                                int64_t ts_to,
                                evidence_record_t *out,
                                size_t max_out,
                                size_t *count);

int  evidence_count(void);

/* Close the evidence store. */
void evidence_close(void);

#ifdef __cplusplus
}
#endif

#endif /* EVIDENCE_STORE_H */
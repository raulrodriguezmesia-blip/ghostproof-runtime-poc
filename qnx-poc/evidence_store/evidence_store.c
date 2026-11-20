/*
 * evidence_store.c - SQLite-backed evidence persistence for GhostProof.
 *
 * Inserts policy decision records into the 'evidence' table and provides
 * simple query helpers.
 */
#include "evidence_store.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static sqlite3 *g_db = NULL;

int evidence_init(const char *db_path)
{
    int rc = sqlite3_open_v2(db_path, &g_db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                             NULL);
    if (rc != SQLITE_OK) return -1;

    char *err = NULL;
    int ret = sqlite3_exec(g_db,
                           "CREATE TABLE IF NOT EXISTS evidence ("
                           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           " timestamp_ns INTEGER NOT NULL,"
                           " actor_id TEXT NOT NULL,"
                           " action_type TEXT NOT NULL,"
                           " resource_id TEXT NOT NULL,"
                           " policy_id TEXT NOT NULL,"
                           " decision TEXT NOT NULL,"
                           " latency_ns INTEGER NOT NULL,"
                           " context_json TEXT);"
                           "CREATE INDEX IF NOT EXISTS idx_timestamp ON evidence (timestamp_ns);"
                           "CREATE INDEX IF NOT EXISTS idx_actor ON evidence (actor_id);"
                           "CREATE INDEX IF NOT EXISTS idx_policy ON evidence (policy_id);",
                           NULL, NULL, &err);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "sqlite exec err: %s\n", err ? err : "unknown");
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

int evidence_insert(const evidence_record_t *rec)
{
    if (!g_db || !rec) return -1;
    sqlite3_stmt *stmt = NULL;
    int ret = sqlite3_prepare_v2(g_db,
        "INSERT INTO evidence (timestamp_ns, actor_id, action_type, resource_id, policy_id, decision, latency_ns, context_json)"
        " VALUES (?1,?2,?3,?4,?5,?6,?7,?8)",
        -1, &stmt, NULL);
    if (ret != SQLITE_OK) return -1;

    sqlite3_bind_int64(stmt, 1, rec->timestamp_ns);
    sqlite3_bind_text(stmt, 2, rec->actor_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, rec->action_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, rec->resource_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, rec->policy_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, rec->decision, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, rec->latency_ns);
    sqlite3_bind_text(stmt, 8, rec->context_json, -1, SQLITE_TRANSIENT);

    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return ret == SQLITE_DONE ? 0 : -1;
}

int evidence_query_by_actor(const char *actor_id,
                             evidence_record_t *out,
                             size_t max_out,
                             size_t *count)
{
    if (!g_db || !actor_id) return -1;
    sqlite3_stmt *stmt = NULL;
    int ret = sqlite3_prepare_v2(g_db,
        "SELECT timestamp_ns, actor_id, action_type, resource_id, policy_id, decision, latency_ns, context_json"
        " FROM evidence WHERE actor_id = ?1 ORDER BY timestamp_ns DESC LIMIT ?2",
        -1, &stmt, NULL);
    if (ret != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, actor_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)max_out);

    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (n >= max_out) break;
        evidence_record_t *r = &out[n];
        r->timestamp_ns = sqlite3_column_int64(stmt, 0);
        strncpy(r->actor_id, (const char*)sqlite3_column_text(stmt,1), EV_ACTOR_LEN-1);
        r->actor_id[EV_ACTOR_LEN-1] = '\0';
        strncpy(r->action_type, (const char*)sqlite3_column_text(stmt,2), EV_ACTION_LEN-1);
        r->action_type[EV_ACTION_LEN-1] = '\0';
        strncpy(r->resource_id, (const char*)sqlite3_column_text(stmt,3), EV_RESOURCE_LEN-1);
        r->resource_id[EV_RESOURCE_LEN-1] = '\0';
        strncpy(r->policy_id, (const char*)sqlite3_column_text(stmt,4), EV_POLICY_LEN-1);
        r->policy_id[EV_POLICY_LEN-1] = '\0';
        strncpy(r->decision, (const char*)sqlite3_column_text(stmt,5), EV_DECISION_LEN-1);
        r->decision[EV_DECISION_LEN-1] = '\0';
        r->latency_ns = sqlite3_column_int64(stmt,6);
        strncpy(r->context_json, (const char*)sqlite3_column_text(stmt,7), EV_CONTEXT_LEN-1);
        r->context_json[EV_CONTEXT_LEN-1] = '\0';
        n++;
    }
    sqlite3_finalize(stmt);
    if (count) *count = n;
    return 0;
}

int evidence_query_by_timestamp(int64_t ts_from, int64_t ts_to,
                                evidence_record_t *out, size_t max_out, size_t *count)
{
    (void)ts_from; (void)ts_to;
    size_t n = 0;
    for (int i = 0; i < (int)max_out; i++) { out[i].timestamp_ns = 0; (void)0; }
    if (count) *count = 0;
    return 0;
}

int evidence_count(void)
{
    if (!g_db) return -1;
    sqlite3_stmt *stmt = NULL;
    int ret = sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM evidence", -1, &stmt, NULL);
    int c = 0;
    if (ret == SQLITE_OK) { sqlite3_step(stmt); c = sqlite3_column_int(stmt, 0); }
    sqlite3_finalize(stmt);
    return c;
}

void evidence_close(void)
{
    if (g_db) { sqlite3_close(g_db); g_db = NULL; }
}

/*
 * schema.sql - GhostProof Validation Experiment
 * Evidence Store schema (SQLite).
 *
 * Every policy decision that passes through the Policy Engine
 * is persisted here as an immutable evidence record.
 */
PRAGMA foreign_keys = OFF;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;

CREATE TABLE IF NOT EXISTS evidence (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_ns    INTEGER NOT NULL,   -- nanoseconds since epoch (CLOCK_REALTIME)
    actor_id        TEXT    NOT NULL,   -- e.g. "agent-001", "controller-brake"
    action_type     TEXT    NOT NULL,   -- e.g. "brake_apply", "throttle_set"
    resource_id     TEXT    NOT NULL,   -- e.g. "brake_system", "ecu_powertrain"
    policy_id       TEXT    NOT NULL,   -- e.g. "POL-SAFETY-01", "POL-DEFAULT-DENY"
    decision        TEXT    NOT NULL,   -- "ALLOW" or "DENY"
    latency_ns      INTEGER NOT NULL,   -- policy evaluation time in ns
    context_json    TEXT                -- additional context (JSON), nullable
);

CREATE INDEX IF NOT EXISTS idx_timestamp ON evidence (timestamp_ns);
CREATE INDEX IF NOT EXISTS idx_actor    ON evidence (actor_id);
CREATE INDEX IF NOT EXISTS idx_policy   ON evidence (policy_id);
CREATE INDEX IF NOT EXISTS idx_decision ON evidence (decision);
CREATE INDEX IF NOT EXISTS idx_resource ON evidence (resource_id);
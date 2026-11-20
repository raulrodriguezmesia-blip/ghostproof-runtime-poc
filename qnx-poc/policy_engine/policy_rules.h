/*
 * policy_rules.h - GhostProof Validation Experiment
 * QNX Neutrino RTOS Policy Engine PoC
 *
 * Policy rule definitions (C struct) used by the Policy Engine.
 * Rules are evaluated in priority order (higher priority first).
 * First matching rule wins.
 */
#ifndef POLICY_RULES_H
#define POLICY_RULES_H

#include <stdint.h>

#define POLICY_ALLOW 1
#define POLICY_DENY  0

#define POLICY_ID_LEN       32
#define PATTERN_LEN         64
#define ACTOR_LIST_LEN      128
#define TIME_WINDOW_LEN     9   /* "23:59:59" + NUL */

typedef struct {
    char policy_id[POLICY_ID_LEN];
    char resource_pattern[PATTERN_LEN];   /* e.g. "brake_*", "*" */
    char action_pattern[PATTERN_LEN];     /* e.g. "*", "read", "write" */
    char actor_allowlist[ACTOR_LIST_LEN]; /* comma-separated, "*" = all */
    char time_window_start[TIME_WINDOW_LEN];
    char time_window_end[TIME_WINDOW_LEN];
    int  decision;   /* POLICY_ALLOW or POLICY_DENY */
    int  priority;   /* higher = evaluated first */
} policy_rule_t;

/*
 * Default policy table.
 * Safety-critical resources are locked down by default (deny),
 * then explicitly opened for trusted actors.
 */
static const policy_rule_t DEFAULT_POLICIES[] = {
    {
        "POL-SAFETY-01",
        "brake_*",
        "*",
        "agent-safety-*",
        "00:00:00",
        "23:59:59",
        POLICY_ALLOW,
        100
    },
    {
        "POL-SAFETY-02",
        "brake_*",
        "*",
        "*",
        "00:00:00",
        "23:59:59",
        POLICY_DENY,
        10
    },
    {
        "POL-DATA-01",
        "ecu_*",
        "read",
        "*",
        "00:00:00",
        "23:59:59",
        POLICY_ALLOW,
        50
    },
    {
        "POL-DATA-02",
        "ecu_*",
        "write",
        "agent-safety-*",
        "00:00:00",
        "23:59:59",
        POLICY_ALLOW,
        50
    },
    {
        "POL-DATA-03",
        "ecu_*",
        "write",
        "*",
        "00:00:00",
        "23:59:59",
        POLICY_DENY,
        10
    },
    {
        "POL-POWER-01",
        "powertrain_*",
        "*",
        "agent-safety-*",
        "00:00:00",
        "23:59:59",
        POLICY_ALLOW,
        50
    },
    {
        "POL-POWER-02",
        "powertrain_*",
        "*",
        "*",
        "00:00:00",
        "23:59:59",
        POLICY_DENY,
        10
    },
};

#define DEFAULT_POLICY_COUNT (sizeof(DEFAULT_POLICIES) / sizeof(DEFAULT_POLICIES[0]))

/*
 * policy_evaluate() is defined in policy_eval.c.
 * Returns 1 = ALLOW, 0 = DENY.
 * Out: matched_policy_id, latency_ns.
 */
int policy_evaluate(const char *actor,
                    const char *action,
                    const char *resource,
                    const char *context_json,
                    char *matched_policy_id,
                    size_t matched_policy_id_size,
                    int64_t *latency_ns);

#endif /* POLICY_RULES_H */
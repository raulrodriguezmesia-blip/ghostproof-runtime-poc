/*
 * policy_eval.c - Policy evaluation logic for GhostProof PoC.
 *
 * Evaluates an actor+action+resource+context tuple against the rule table
 * and returns ALLOW/DENY with the matched policy id and the elapsed
 * evaluation time in nanoseconds.
 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "policy_rules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
/* Portable monotonic clock: QueryPerformanceCounter on Windows. */
static int64_t gp_now_ns(void)
{
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    /* convert to ns avoiding overflow: (c * 1e9) / freq  -> use 128-bit math */
    return (int64_t)((double)c.QuadPart * 1e9 / (double)freq.QuadPart);
}
#else
/* Portable monotonic clock: clock_gettime(CLOCK_MONOTONIC) on QNX/Linux. */
static int64_t gp_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
#endif

/* Match a string against a glob-like pattern where '*' matches any
 * sequence and '?' matches any single character. */
static int pattern_match(const char *pattern, const char *value)
{
    if (pattern == NULL || value == NULL) return 0;

    while (*pattern) {
        if (*pattern == '*') {
            /* skip consecutive '*' */
            while (*pattern == '*') pattern++;
            if (*pattern == '\0') return 1;  /* trailing '*' matches rest */
            while (*value) {
                if (pattern_match(pattern, value)) return 1;
                value++;
            }
            return pattern_match(pattern, value);
        } else if (*pattern == '?') {
            if (*value == '\0') return 0;
            pattern++;
            value++;
        } else {
            if (*pattern != *value) return 0;
            pattern++;
            value++;
        }
    }
    return *value == '\0';
}

/* Check if an actor is in a comma-separated allowlist. "*" matches all.
 * Each token may itself be a glob pattern (e.g. "agent-safety-*"). */
static int actor_in_allowlist(const char *allowlist, const char *actor)
{
    if (allowlist == NULL || actor == NULL) return 0;

    /* wildcard */
    if (strcmp(allowlist, "*") == 0) return 1;

    char tmp[ACTOR_LIST_LEN];
    strncpy(tmp, allowlist, ACTOR_LIST_LEN - 1);
    tmp[ACTOR_LIST_LEN - 1] = '\0';

    char *saveptr = NULL;
    char *tok = strtok_r(tmp, ",", &saveptr);
    while (tok) {
        /* trim leading spaces */
        while (*tok == ' ') tok++;
        /* each token may be a glob pattern */
        if (pattern_match(tok, actor)) return 1;
        tok = strtok_r(NULL, ",", &saveptr);
    }
    return 0;
}

/* Evaluate a single policy request against the rule table.
 *
 * Returns: 1 = ALLOW, 0 = DENY
 * Out params: matched_policy_id, latency_ns
 */
int policy_evaluate(const char *actor,
                    const char *action,
                    const char *resource,
                    const char *context_json,
                    char *matched_policy_id,
                    size_t matched_policy_id_size,
                    int64_t *latency_ns)
{
    (void)context_json; /* reserved for future context-aware rules */

    int64_t t0 = 0, t1 = 0;

    /* capture start timestamp */
    t0 = gp_now_ns();

    const policy_rule_t *rules = DEFAULT_POLICIES;
    size_t n = DEFAULT_POLICY_COUNT;

    /* Sort by priority descending: pick highest priority first match.
     * The table is already ordered by priority desc, but we scan
     * and track the best match to be robust. */
    int best_idx = -1;
    int best_priority = -1;

    for (size_t i = 0; i < n; i++) {
        if (rules[i].priority < best_priority) continue; /* already found higher */

        if (!actor_in_allowlist(rules[i].actor_allowlist, actor))
            continue;
        if (!pattern_match(rules[i].resource_pattern, resource))
            continue;
        if (!pattern_match(rules[i].action_pattern, action))
            continue;

        /* first acceptable rule at this priority level wins */
        best_idx = (int)i;
        best_priority = rules[i].priority;
        break;
    }

    int decision;
    const char *pid;

    if (best_idx >= 0) {
        decision = rules[best_idx].decision;
        pid = rules[best_idx].policy_id;
    } else {
        /* No rule matched -> default deny (safe default) */
        decision = POLICY_DENY;
        pid = "POL-DEFAULT-DENY";
    }

    /* capture end timestamp */
    t1 = gp_now_ns();

    if (latency_ns) *latency_ns = t1 - t0;
    if (matched_policy_id && matched_policy_id_size > 0) {
        strncpy(matched_policy_id, pid, matched_policy_id_size - 1);
        matched_policy_id[matched_policy_id_size - 1] = '\0';
    }

    return decision;
}
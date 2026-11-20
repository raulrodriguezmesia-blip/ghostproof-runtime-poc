/*
 * latency_test.c - GhostProof Validation Experiment
 * Latency Benchmark: 1000 iterations of policy evaluation.
 *
 * Mede p50, p95, p99 de la evaluación de políticas y compara con la
 * hipótesis de GhostProof (< 100 µs).
 *
 * Build: gcc -O2 -o latency_test latency_test.c policy_eval.c -lm
 * Run:   ./latency_test
 */
#include "policy_rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define ITERATIONS 1000

static int cmp_ll(const void *a, const void *b)
{
    int64_t x = *(const int64_t*)a;
    int64_t y = *(const int64_t*)b;
    return (x > y) - (x < y);
}

int main(void)
{
    int64_t *latencies = (int64_t*)malloc(sizeof(int64_t) * ITERATIONS);
    if (!latencies) { fprintf(stderr, "malloc failed\n"); return 1; }

    const char *actors[] = {
        "agent-safety-01", "agent-telemetry", "agent-safety-02",
        "agent-telem-02", "agent-telem-03", "agent-safety-03",
        "agent-power-01", "agent-maint", "unknown-agent"
    };
    const char *actions[] = {
        "brake_apply", "brake_release", "read", "write", "set", "unknown"
    };
    const char *resources[] = {
        "brake_front", "brake_rear", "ecu_engine", "powertrain_torque",
        "unknown_resource"
    };

    int n_actors = sizeof(actors)/sizeof(actors[0]);
    int n_actions = sizeof(actions)/sizeof(actions[0]);
    int n_resources = sizeof(resources)/sizeof(resources[0]);

    int64_t sum_ns = 0, min_ns = INT64_MAX, max_ns = INT64_MIN;
    int allow_count = 0, deny_count = 0;

    for (int i = 0; i < ITERATIONS; i++) {
        const char *actor = actors[i % n_actors];
        const char *action = actions[(i * 3) % n_actions];
        const char *resource = resources[(i * 7) % n_resources];

        char matched_policy[32];
        int64_t latency_ns = 0;

        int decision = policy_evaluate(actor, action, resource, NULL,
                                       matched_policy, sizeof(matched_policy),
                                       &latency_ns);

        latencies[i] = latency_ns;
        sum_ns += latency_ns;
        if (latency_ns < min_ns) min_ns = latency_ns;
        if (latency_ns > max_ns) max_ns = latency_ns;
        if (decision == POLICY_ALLOW) allow_count++; else deny_count++;
    }

    qsort(latencies, ITERATIONS, sizeof(int64_t), cmp_ll);

    double p50 = latencies[(ITERATIONS/2)];
    double p95 = latencies[(int)(ITERATIONS * 0.95)];
    double p99 = latencies[(int)(ITERATIONS * 0.99)];
    double mean_ns = (double)sum_ns / ITERATIONS;

    /* variance */
    double sum_sq = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        double d = latencies[i] - mean_ns;
        sum_sq += d * d;
    }
    double stddev_ns = sqrt(sum_sq / ITERATIONS);

    printf("=== GhostProof Policy Engine Latency Benchmark ===\n");
    printf("Iterations:        %d\n", ITERATIONS);
    printf("Mean:              %.2f ns\n", mean_ns);
    printf("Min:               %lld ns\n", (long long)min_ns);
    printf("Max:               %lld ns\n", (long long)max_ns);
    printf("Stddev:            %.2f ns\n", stddev_ns);
    printf("p50:               %.0f ns\n", p50);
    printf("p95:               %.0f ns\n", p95);
    printf("p99:               %.0f ns\n", p99);
    printf("Allow decisions:   %d\n", allow_count);
    printf("Deny decisions:    %d\n", deny_count);
    printf("\n");
    printf("Hypothesis: policy eval < 100,000 ns (100 us)\n");
    printf("p99 result:         %s\n", (p99 < 100000.0) ? "PASS" : "FAIL");
    printf("p95 result:         %s\n", (p95 < 100000.0) ? "PASS" : "FAIL");
    printf("p50 result:         %s\n", (p50 < 100000.0) ? "PASS" : "FAIL");

    free(latencies);
    return (p99 < 100000.0) ? 0 : 1;
}
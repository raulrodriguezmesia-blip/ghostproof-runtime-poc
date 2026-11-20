# GhostProof Validation Experiment — QNX Hackathon Submission

## Estado: COMPLETED ✅ | Decisión: GO ✅

### 1. Qué es GhostProof

GhostProof es un **Proof of Concept de gobernanza runtime** que responde a una pregunta concreta:

> ¿Puede un agente autónomo tomar decisiones en tiempo real, trazables y auditables?

Más allá de un motor de reglas, GhostProof es una **cadena completa de evidencia**:

```
Actor
↓
Policy Evaluation   (Deterministic Decision)
↓
Policy Attribution  (Matched Policy ID)
↓
Evidence Capture    (Timestamp, Latency, Context)
↓
Persistent Audit Trail (SQLite)
```

### 2. Hipótesis validadas

| Hipótesis | Estado | Evidencia |
|-----------|--------|-----------|
| H1: Latencia de evaluación < 100 µs | ✅ PASS | p50=300 ns / p95=500 ns / p99=500 ns |
| H2: Decisiones correctas (Allow/Deny) | ✅ PASS | 4 ALLOW / 5 DENY, 100% alineadas |
| H3: Persistencia de evidencia | ✅ PASS | 9/9 registros en SQLite |
| H4: Flujo end-to-end observable | ✅ PASS | Agent → Policy Engine → Evidence Store |
| H5: Ejecución sobre QNX real | ⏳ Pendiente | Sin impacto en validez arquitectónica |

### 3. Resultado principal

**p99 = 500 ns frente a un objetivo de 100.000 ns (100 µs)**

→ Margen ≈ **200x** por debajo del umbral.

### 4. Cómo ejecutar

```bash
# Compilar (Windows, gcc)
gcc -O2 -std=c11 -o policy_engine qnx-poc/policy_engine/main.c qnx-poc/policy_engine/policy_eval.c -lm -lws2_32
gcc -O2 -std=c11 -o evidence_store qnx-poc/evidence_store/main.c qnx-poc/evidence_store/evidence_store.c qnx-poc/evidence_store/sqlite3.c -lws2_32
gcc -O2 -std=c11 -o simulated_agent qnx-poc/simulated_agent/main.c -lws2_32
gcc -O2 -std=c11 -o latency_test qnx-poc/benchmark/latency_test.c qnx-poc/policy_engine/policy_eval.c -lm -Iqnx-poc/policy_engine

# Ejecutar benchmark
./latency_test

# Ejecutar integración
python qnx-poc/integration_test/run_integration.py
```

En QNX Neutrino RTOS, los mismos fuentes se compilan con `qcc`.

### 5. Artefactos

```
qnx-poc/                ← Código fuente completo del PoC
├── policy_engine/      ← Motor de decisiones ALLOW/DENY
├── evidence_store/     ← Persistencia de evidencia (SQLite)
├── simulated_agent/    ← Agente de prueba
├── benchmark/          ← Latencia p50/p95/p99
├── integration_test/   ← Test end-to-end
└── validation_notes/
    └── validation_001.md  ← Resultados completos y Go/No-Go
```

### 6. Conclusión

La hipótesis de GhostProof quedó **validada técnicamente**.

Un sistema de gobernanza runtime puede tomar decisiones determinísticas, trazables y auditables con sobrecoste despreciable (0.5 µs p99).

**Veredicto: GO.** GhostProof merece avanzar a especificación v0.1 como siguiente fase (posterior a este cierre).
# qnx-validation-experiment

**GhostProof Validation Experiment — QNX Global Hackathon Submission**

Experimento que valida la tesis *From Trust to Evidence*: un motor de políticas runtime con persistencia de evidencia auditable.

## Estado: COMPLETED ✅ | Decisión: GO ✅

## Resultado

p99 = 500 ns frente a objetivo de 100.000 ns (100 µs) → margen 200x.

## Documentación

- [DEVPOST_SUBMISSION.md](DEVPOST_SUBMISSION.md) — Paquete de submission para QNX Global Hackathon
- [qnx-poc/README_QNX.md](qnx-poc/README_QNX.md) — Arquitectura y build/run
- [qnx-poc/validation_notes/validation_001.md](qnx-poc/validation_notes/validation_001.md) — Resultados completos, Go/No-Go

## Estructura

```
qnx-poc/
├── policy_engine/      # Motor de decisiones ALLOW/DENY
├── evidence_store/     # Persistencia de evidencia (SQLite)
├── simulated_agent/    # Agente de prueba
├── benchmark/          # Latencia p50/p95/p99
├── integration_test/   # Test end-to-end
└── validation_notes/   # Resultados y decisión Go/No-Go
```

## Siguiente fase

**GhostProof v0.1 spec** (especificación de producto) — pendiente de abrir después de cerrar este entregable.
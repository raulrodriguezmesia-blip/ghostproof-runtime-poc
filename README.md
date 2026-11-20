# QNX Validation Experiment

**GhostProof Validation Experiment — QNX Global Hackathon Submission**

Experimento que valida la tesis *From Trust to Evidence*: un motor de políticas en tiempo de ejecución con persistencia de evidencia auditable.

Estado: COMPLETADO ✅ | Decisión: GO ✅

## Resultado

Latencia de evaluación: **p99 = 500 ns** frente a objetivo de **100.000 ns (100 µs)** → ~200x de margen en las pruebas ejecutadas.
Integración y persistencia: **9/9 escenarios end-to-end verificados y 9/9 registros persistidos en SQLite**.
Portabilidad QNX real (qcc / QNX SDP 8.0): Validación de plataforma pendiente como paso no bloqueante de cara al cierre del hackathon (15 de diciembre de 2026).

## Documentación

- [DEVPOST_SUBMISSION.md](DEVPOST_SUBMISSION.md) — Paquete de envío para QNX Global Hackathon
- [qnx-poc/README_QNX.md](qnx-poc/README_QNX.md) — Arquitectura, compilación y ejecución
- [qnx-poc/validation_notes/validation_001.md](qnx-poc/validation_notes/validation_001.md) — Resultados completos y decisión Go/No-Go

## Arquitectura

```text
Agent
  ↓
Policy Engine
  ↓
ALLOW / DENY
  ↓
Evidence Store
  ↓
SQLite Audit Trail
```

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

## Resultados de validación

| Hipótesis                        | Estado |
|----------------------------------|--------|
| H1: Latencia < 100 µs            | ✅ PASS |
| H2: Decisiones correctas         | ✅ PASS |
| H3: Persistencia de evidencia    | ✅ PASS |
| H4: Integración end-to-end       | ✅ PASS |
| H5: Ejecución en QNX real        | ⏳ Pendiente |

## Licencia

MIT License.

## Estado del proyecto

El experimento de validación está cerrado.
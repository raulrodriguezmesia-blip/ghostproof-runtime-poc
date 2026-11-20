# GhostProof Validation Experiment — QNX PoC

**QNX Global Hackathon Submission**

Experimento que valida la hipótesis de GhostProof: un motor de políticas de tiempo de ejecución que evalúa acciones y persiste evidencia (audit trail) en QNX Neutrino RTOS.

## Arquitectura

```
┌─────────────────┐   msg passing   ┌──────────────────┐
│ Simulated Agent │ ──────────────▶ │  Policy Engine   │
│   (main.c)      │                 │ (main + eval)    │
└─────────────────┘                 └──────┬───────────┘
                                            │ msg passing
                                            ▼
                              ┌──────────────────┐
                              │  Evidence Store  │
                              │  (SQLite)        │
                              └──────────────────┘
```

Componentes:
- `policy_engine/` — recibe acciones, evalúa políticas, decide Allow/Deny.
- `evidence_store/` — persistencia SQLite de cada decisión.
- `simulated_agent/` — genera acciones de prueba.
- `benchmark/` — benchmark de latencia (p50/p95/p99).
- `validation_notes/` — resultados y decisión Go/No-Go.

## Estructura

```
qnx-poc/
├── policy_engine/      # Motor de políticas
│   ├── main.c          # Servidor TCP + message loop
│   ├── policy_eval.c   # Lógica de evaluación
│   ├── policy_rules.h  # Tabla de reglas
│   └── Makefile
├── evidence_store/     # Persistencia de evidencia
│   ├── main.c          # Servidor TCP + SQLite
│   ├── evidence_store.c/h
│   ├── schema.sql      # CREATE TABLE
│   └── Makefile
├── simulated_agent/    # Agente simulado
│   ├── main.c
│   └── Makefile
├── benchmark/          # Medición de latencia
│   ├── latency_test.c
│   └── Makefile
├── integration_test/   # Test end-to-end
│   └── run_integration.py
└── validation_notes/
    └── validation_001.md
```

## Build (host: Linux/Windows con gcc)

```bash
make -C policy_engine all
make -C evidence_store all
make -C simulated_agent all
make -C benchmark all
```

## Ejecución

### Benchmark de latencia
```bash
make -C benchmark run
```
Muestra p50/p95/p99 y compara con la hipótesis (< 100 µs).

### Test de integración end-to-end
```bash
python integration_test/run_integration.py
```
Arranca los servicios, ejecuta el agente y verifica la persistencia en SQLite.

## QNX Neutrino RTOS (simulador SDP 8.0)

Los fuentes son portables. En QNX, compila con `qcc`:
```bash
qcc -Vgcc_ntox86_64 -o policy_engine main.c policy_eval.c
qcc -Vgcc_ntox86_64 -o evidence_store main.c evidence_store.c -lsqlite3
```
Los sockets TCP y SQLite son nativos en QNX.

## Hipótesis

> El motor de políticas evalúa una acción y decide Allow/Deny en **< 100 µs (p99)**.

## Entregables

- `validation_notes/validation_001.md` — latencias, learnings, Go/No-Go.
- `benchmark/latency_test.c` — benchmark de 1000 iteraciones.
- `integration_test/run_integration.py` — test end-to-end observable.
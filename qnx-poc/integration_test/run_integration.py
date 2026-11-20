#!/usr/bin/env python3
"""
run_integration.py - GhostProof Integration Test (host harness)

Orquesta el flujo end-to-end:
  Simulated Agent -> Policy Engine -> Evidence Store -> SQLite -> Log

Arranca los servicios C (policy_engine, evidence_store), ejecuta el agente
y verifica que las decisiones quedaron persistidas en la base de datos.

Uso:
  python run_integration.py
  python run_integration.py --bin-dir ../..   # si los binarios están en otro lado
"""
import argparse
import os
import socket
import sqlite3
import subprocess
import sys
import time
from pathlib import Path

POLICY_PORT = 9090
EVIDENCE_PORT = 9091


def wait_port(host, port, timeout=5.0):
    """Espera hasta que el puerto acepte conexiones."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.3):
                return True
        except OSError:
            time.sleep(0.05)
    return False


def main():
    here = Path(__file__).resolve().parent
    root = here.parent  # qnx-poc/

    ap = argparse.ArgumentParser()
    ap.add_argument("--policy-bin", default=str(root / "policy_engine" / "policy_engine"))
    ap.add_argument("--evidence-bin", default=str(root / "evidence_store" / "evidence_store"))
    ap.add_argument("--agent-bin", default=str(root / "simulated_agent" / "simulated_agent"))
    ap.add_argument("--db", default=str(root / "evidence.db"))
    args = ap.parse_args()

    for p in (args.policy_bin, args.evidence_bin, args.agent_bin):
        if not os.path.exists(p) and not os.path.exists(p + ".exe"):
            print(f"[FATAL] binario no encontrado: {p}")
            print("        Compila primero (ver README_QNX.md).")
            return 2

    procs = []
    try:
        print("[1/5] Arrancando Evidence Store ...")
        ev = subprocess.Popen([args.evidence_bin], cwd=str(root))
        procs.append(ev)
        if not wait_port("127.0.0.1", EVIDENCE_PORT):
            print("[FATAL] Evidence Store no respondió en puerto", EVIDENCE_PORT)
            return 3

        print("[2/5] Arrancando Policy Engine ...")
        pe = subprocess.Popen([args.policy_bin], cwd=str(root))
        procs.append(pe)
        if not wait_port("127.0.0.1", POLICY_PORT):
            print("[FATAL] Policy Engine no respondió en puerto", POLICY_PORT)
            return 3

        print("[3/5] Ejecutando Simulated Agent ...")
        agent = subprocess.run([args.agent_bin], cwd=str(root), capture_output=True, text=True)
        print(agent.stdout)
        if agent.returncode != 0:
            print("[WARN] agent return code:", agent.returncode)

        print("[4/5] Esperando flush de evidencia ...")
        time.sleep(0.5)

        print("[5/5] Verificando Evidence Store (SQLite) ...")
        if not os.path.exists(args.db):
            print("[FAIL] base de datos no creada:", args.db)
            return 4

        conn = sqlite3.connect(args.db)
        cur = conn.cursor()
        cur.execute("SELECT COUNT(*) FROM evidence")
        total = cur.fetchone()[0]
        cur.execute("SELECT decision, COUNT(*) FROM evidence GROUP BY decision")
        by_decision = dict(cur.fetchall())
        cur.execute("SELECT actor_id, action_type, resource_id, policy_id, decision, latency_ns "
                    "FROM evidence ORDER BY id DESC LIMIT 5")
        rows = cur.fetchall()
        conn.close()

        print()
        print("=== INTEGRATION TEST RESULT ===")
        print(f"Registros persistidos: {total}")
        print(f"Por decisión: {by_decision}")
        print("Últimos 5 registros:")
        for r in rows:
            print("  ", r)

        ok = total > 0 and "ALLOW" in by_decision and "DENY" in by_decision
        print()
        print("RESULTADO:", "PASS" if ok else "FAIL")
        return 0 if ok else 5

    finally:
        for p in procs:
            try:
                p.terminate()
            except Exception:
                pass
        for p in procs:
            try:
                p.wait(timeout=3)
            except Exception:
                try:
                    p.kill()
                except Exception:
                    pass


if __name__ == "__main__":
    sys.exit(main())
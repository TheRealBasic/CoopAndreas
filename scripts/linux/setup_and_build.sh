#!/usr/bin/env bash
set -euo pipefail

MODE="${MODE:-release}"
RUN_SERVER="${RUN_SERVER:-0}"

if [[ "${EUID}" -ne 0 ]]; then
  SUDO="sudo"
else
  SUDO=""
fi

echo "[INFO] Installing build dependencies (Ubuntu/Debian)..."
${SUDO} apt-get update -y
${SUDO} apt-get install -y build-essential clang cmake git xmake

echo "[INFO] Toolchain versions"
g++ --version | head -n 1
clang --version | head -n 1
cmake --version | head -n 1
XMAKE_ROOT=y xmake --version | head -n 1

echo "[INFO] Configuring xmake mode: ${MODE}"
XMAKE_ROOT=y xmake f -m "${MODE}"

echo "[INFO] Building server target"
XMAKE_ROOT=y xmake --build server

SERVER_BIN="./build/linux/x86_64/${MODE}/server"
if [[ -x "${SERVER_BIN}" ]]; then
  echo "[OK] Built server binary: ${SERVER_BIN}"
else
  echo "[ERROR] Server binary was not found at ${SERVER_BIN}" >&2
  exit 1
fi

if [[ "${RUN_SERVER}" == "1" ]]; then
  echo "[INFO] Starting server"
  exec "${SERVER_BIN}"
fi

echo "[OK] Done. Set RUN_SERVER=1 to launch automatically after build."

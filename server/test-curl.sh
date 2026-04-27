#!/usr/bin/env bash
set -euo pipefail

TELEGRAM_ID="${1:-}"
PORT="${SERVER_PORT:-48731}"

if [ -f .env ]; then
  # shellcheck disable=SC1091
  source .env
  PORT="${SERVER_PORT:-48731}"
fi

if [ -z "$TELEGRAM_ID" ]; then
  echo "Usage: ./test-curl.sh YOUR_TELEGRAM_ID"
  exit 1
fi

curl -sS -X POST "http://127.0.0.1:${PORT}/telegram/test" \
  -H "Content-Type: application/json" \
  -d "{\"telegram_id\":\"${TELEGRAM_ID}\",\"device_id\":\"esp-oob-test\",\"device_name\":\"ESP-OOB Test Device\"}"

echo

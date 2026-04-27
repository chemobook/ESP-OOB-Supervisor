#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

echo "ESP-OOB Telegram Server installer"
echo

if [ -f .env ]; then
  echo ".env already exists. I will not overwrite it."
  echo "Edit it manually if needed: nano .env"
else
  read -r -p "Enter Telegram bot token from @BotFather: " TELEGRAM_BOT_TOKEN
  read -r -p "Enter external server port [48731]: " SERVER_PORT
  SERVER_PORT="${SERVER_PORT:-48731}"

  cat > .env <<ENVEOF
TELEGRAM_BOT_TOKEN=${TELEGRAM_BOT_TOKEN}
SERVER_PORT=${SERVER_PORT}
ENVEOF

  chmod 600 .env
  echo ".env created."
fi

docker compose up -d --build

echo
echo "Server started."
echo "Health check: curl http://127.0.0.1:${SERVER_PORT:-48731}/health"
echo "Telegram test: ./test-curl.sh YOUR_TELEGRAM_ID"

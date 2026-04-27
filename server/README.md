# ESP-OOB Telegram Server

Minimal server part for ESP-OOB-Supervisor Telegram concept test.

The Telegram bot token is **not stored in GitHub**. It is written only to `.env` on your VPS during installation.

## Files

```text
server/
├── Dockerfile
├── docker-compose.yml
├── app.py
├── requirements.txt
├── .env.example
├── .gitignore
├── install.sh
├── README.md
└── test-curl.sh
```

## Installation on VPS

```bash
cd server
chmod +x install.sh test-curl.sh
./install.sh
```

The installer asks for:

```text
Telegram bot token from @BotFather
External server port, default 48731
```

It creates local `.env`:

```env
TELEGRAM_BOT_TOKEN=your_real_token_here
SERVER_PORT=48731
```

`.env` is ignored by Git and must not be uploaded to GitHub.

## Manual installation

```bash
cd server
cp .env.example .env
nano .env
docker compose up -d --build
```

## Check server

```bash
curl http://127.0.0.1:48731/health
```

## Test Telegram delivery

Important: the user must open the bot in Telegram and press `/start` first. Telegram bots cannot start a private chat first.

```bash
./test-curl.sh YOUR_TELEGRAM_ID
```

Or manually:

```bash
curl -X POST http://127.0.0.1:48731/telegram/test \
  -H "Content-Type: application/json" \
  -d '{
    "telegram_id": "123456789",
    "device_id": "esp-oob-001",
    "device_name": "Remote Supervisor"
  }'
```

## Endpoint for ESP32 web UI button

Button: **Check Telegram** / **Проверить Telegram**

ESP32 sends:

```http
POST http://YOUR_SERVER_IP:48731/telegram/test
Content-Type: application/json
```

Body:

```json
{
  "telegram_id": "123456789",
  "device_id": "esp-oob-001",
  "device_name": "Remote Supervisor"
}
```

Successful server response:

```json
{
  "ok": true,
  "message": "telegram_test_sent"
}
```

Telegram message text:

```text
✅ ESP-OOB-Supervisor connected
Device ID: esp-oob-001
Device name: Remote Supervisor
Request IP: x.x.x.x

Telegram test message delivered successfully.
```

## Send custom message

```http
POST /telegram/send
```

Body:

```json
{
  "telegram_id": "123456789",
  "device_id": "esp-oob-001",
  "device_name": "Remote Supervisor",
  "text": "Alarm test from device"
}
```

## Current security model

This is a concept-test version.

There is no per-device API key yet. Anyone who knows the server address and port can try to send a message through your bot. For a private test this is acceptable for a short time, but for a public/release version add at least one of these:

- device claim token;
- per-device secret generated after pairing;
- Telegram confirmation flow;
- allowlist of known device IDs;
- HTTPS reverse proxy;
- rate limiting.

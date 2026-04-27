import os
import time
from typing import Optional

import requests
from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel, Field

TELEGRAM_BOT_TOKEN = os.getenv("TELEGRAM_BOT_TOKEN", "").strip()
SERVER_PORT = int(os.getenv("SERVER_PORT", "48731"))

app = FastAPI(title="ESP-OOB Telegram Server", version="0.3.0")


class TelegramTestRequest(BaseModel):
    telegram_id: str = Field(..., min_length=3, max_length=32)
    device_id: Optional[str] = Field(default="unknown", max_length=64)
    device_name: Optional[str] = Field(default="ESP-OOB-Supervisor", max_length=96)


class TelegramMessageRequest(BaseModel):
    telegram_id: str = Field(..., min_length=3, max_length=32)
    text: str = Field(..., min_length=1, max_length=3500)
    device_id: Optional[str] = Field(default="unknown", max_length=64)
    device_name: Optional[str] = Field(default="ESP-OOB-Supervisor", max_length=96)


def require_bot_token() -> None:
    if not TELEGRAM_BOT_TOKEN or TELEGRAM_BOT_TOKEN == "PASTE_YOUR_TELEGRAM_BOT_TOKEN_HERE":
        raise HTTPException(status_code=500, detail="TELEGRAM_BOT_TOKEN is not configured")


def send_telegram_message(chat_id: str, text: str) -> dict:
    require_bot_token()
    url = f"https://api.telegram.org/bot{TELEGRAM_BOT_TOKEN}/sendMessage"
    payload = {
        "chat_id": chat_id,
        "text": text,
        "parse_mode": "HTML",
        "disable_web_page_preview": True,
    }
    try:
        response = requests.post(url, json=payload, timeout=15)
    except requests.RequestException as exc:
        raise HTTPException(status_code=502, detail=f"Telegram API connection error: {exc}") from exc

    data = response.json() if response.content else {}
    if not response.ok or not data.get("ok"):
        description = data.get("description", "Unknown Telegram API error")
        raise HTTPException(status_code=502, detail=description)
    return data


@app.get("/health")
def health() -> dict:
    return {
        "ok": True,
        "service": "esp-oob-telegram-server",
        "version": "0.3.0",
        "telegram_token_configured": bool(TELEGRAM_BOT_TOKEN and TELEGRAM_BOT_TOKEN != "PASTE_YOUR_TELEGRAM_BOT_TOKEN_HERE"),
        "time": int(time.time()),
    }


@app.post("/telegram/test")
def telegram_test(body: TelegramTestRequest, request: Request) -> dict:
    ip = request.client.host if request.client else "unknown"
    text = (
        "✅ <b>ESP-OOB-Supervisor connected</b>\n\n"
        f"Device ID: <code>{body.device_id}</code>\n"
        f"Device name: <code>{body.device_name}</code>\n"
        f"Request IP: <code>{ip}</code>\n\n"
        "Telegram test message delivered successfully."
    )
    send_telegram_message(body.telegram_id, text)
    return {"ok": True, "message": "telegram_test_sent"}


@app.post("/telegram/send")
def telegram_send(body: TelegramMessageRequest) -> dict:
    text = (
        f"📡 <b>{body.device_name}</b>\n"
        f"Device ID: <code>{body.device_id}</code>\n\n"
        f"{body.text}"
    )
    send_telegram_message(body.telegram_id, text)
    return {"ok": True, "message": "telegram_message_sent"}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="0.0.0.0", port=48731)

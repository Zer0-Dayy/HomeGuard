import os
import time
import json
import logging
import threading
import uuid
import smtplib
import re

from email.mime.text import MIMEText
from flask import Flask, request, jsonify
from dotenv import load_dotenv
import requests

# ======================= CONFIG / ENV =======================

load_dotenv()

API_KEY   = os.getenv("API_KEY", "")

SMTP_HOST = os.getenv("SMTP_HOST", "")
SMTP_PORT = int(os.getenv("SMTP_PORT", "587"))
SMTP_USER = os.getenv("SMTP_USER", "")
SMTP_PASS = os.getenv("SMTP_PASS", "")

EMAIL_TO_USER      = os.getenv("EMAIL_TO_USER", "")
EMAIL_TO_EMERGENCY = os.getenv("EMAIL_TO_EMERGENCY", "")

OLLAMA_URL   = os.getenv("OLLAMA_URL", "http://localhost:11434/api/generate")
OLLAMA_MODEL = os.getenv("OLLAMA_MODEL", "gpt-oss:20b")

LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO").upper()
PORT      = int(os.getenv("SERVER_PORT", "5000"))

logging.basicConfig(
    level=getattr(logging, LOG_LEVEL, logging.INFO),
    format="%(asctime)s [%(levelname)s] %(message)s",
)

app = Flask(__name__)
JOBS = {}

# ======================= EMAIL SENDER =======================

def send_email(subject: str, body: str, to_addr: str) -> bool:
    if not (SMTP_HOST and SMTP_USER and SMTP_PASS and to_addr):
        logging.warning("Email config incomplete; skipping email send.")
        return False

    msg = MIMEText(body)
    msg["Subject"] = subject[:200]
    msg["From"] = SMTP_USER
    msg["To"] = to_addr

    try:
        with smtplib.SMTP(SMTP_HOST, SMTP_PORT) as s:
            s.starttls()
            s.login(SMTP_USER, SMTP_PASS)
            s.sendmail(SMTP_USER, [to_addr], msg.as_string())
        logging.info(f"Email sent to {to_addr}.")
        return True
    except Exception as e:
        logging.error(f"Email send failed: {e}")
        return False

# ======================= MODEL PROMPT =======================

PROMPT = """
You are an autonomous safety-analysis system for a home rover.

You MUST output a single JSON object ONLY, inside these tags:

<json>
{ ... }
</json>

Nothing is allowed before <json> or after </json>. Violating this rule causes override.

JSON schema:
{
  "action": "none" | "send_email",
  "email_recipient": "user" | "emergency" | "both",
  "email_subject": "string",
  "email_body": "string",
  "severity": "info" | "warning" | "emergency"
}

Rules:
- EMERGENCY if any reading has:
  - temperature > 70 °C
  - or gas > 1.0
- EMERGENCY always requires sending email.
- email_body must be ≤120 words.
- For safe readings: you MAY choose action="none" (the local system will handle summaries).

Readings:
{block}
"""

# ======================= OLLAMA DECISION ENGINE =======================

def ollama_decision(rows):
    trimmed = rows[-40:]
    block = json.dumps(trimmed, ensure_ascii=False, indent=2)

    payload = {
        "model": OLLAMA_MODEL,
        "prompt": PROMPT.replace("{block}", block),
        "stream": False,
    }

    try:
        res = requests.post(OLLAMA_URL, json=payload, timeout=200)
    except Exception as e:
        logging.error(f"Ollama unreachable: {e}")
        return {"action": "none", "severity": "info", "error": str(e)}

    try:
        js = res.json()
        text = js.get("response", "") or res.text
    except Exception:
        text = res.text

    logging.debug("RAW MODEL OUTPUT:\n" + text)

    m = re.search(r"<json>(.*?)</json>", text, re.S)
    if not m:
        logging.error("Model did not return JSON.")
        return {"action": "none", "severity": "info"}

    try:
        return json.loads(m.group(1))
    except Exception as e:
        logging.error(f"JSON parse failed: {e}")
        return {"action": "none", "severity": "info"}

# ======================= DATA NORMALIZATION =======================

def normalize_packet(packet):
    if isinstance(packet, dict) and "records" in packet:
        readings = packet["records"]
    elif isinstance(packet, dict) and all(k in packet for k in ("timestamp", "temperature", "humidity", "gas")):
        readings = [packet]
    else:
        raise ValueError("Invalid JSON format")

    rows = []
    for r in readings:
        try:
            rows.append({
                "timestamp": r["timestamp"],
                "temperature": float(r["temperature"]),
                "humidity": float(r["humidity"]),
                "gas": float(r["gas"]),
            })
        except:
            pass

    if not rows:
        raise ValueError("All readings invalid")

    return rows

# ======================= LOCAL EMERGENCY FALLBACK =======================

def fallback_emergency(rows):
    last = rows[-1]
    t = last["temperature"]
    g = last["gas"]
    h = last["humidity"]

    if t > 70 or g > 1.0:
        logging.warning("LOCAL EMERGENCY TRIGGERED.")

        body = (
            f"Emergency detected!\n"
            f"Timestamp: {last['timestamp']}\n"
            f"Temperature: {t} °C\n"
            f"Humidity: {h} %\n"
            f"Gas: {g}\n"
            f"Action Required: Overheating or gas hazard."
        )

        send_email("Emergency Alert", body, EMAIL_TO_EMERGENCY)
        return True

    return False

# ======================= SAFE SUMMARY GENERATOR (S2 + recommendations) =======================

def build_safe_summary(last):
    t = last["temperature"]
    h = last["humidity"]
    g = last["gas"]

    rec = []

    if t < 18:
        rec.append("Increase room temperature for better device performance.")
    if 18 <= t <= 28:
        rec.append("Temperature is optimal for safe operation.")
    if h < 30:
        rec.append("Consider running a humidifier for sensor stability.")
    if h > 70:
        rec.append("Reduce humidity levels to prevent condensation.")
    if 0.5 < g <= 1.0:
        rec.append("Improve ventilation to keep gas levels low.")
    if g <= 0.5:
        rec.append("Gas levels are excellent. No ventilation needed.")
    if g > 0.8:
        rec.append("Open windows for better gas dissipation.")

    recommendations = "\n- ".join(rec)

    return f"""
Safety Check: NORMAL

The rover is operating within safe limits.
No dangerous temperature or gas levels were detected.

Latest reading:
Timestamp: {last['timestamp']}
Temperature: {t} °C
Humidity: {h} %
Gas index: {g}

Recommendations:
- {recommendations}
""".strip()

# ======================= ANALYSIS JOB =======================

def analyze_job(job_id, rows):
    try:
        JOBS[job_id] = {"status": "running", "summary": "", "error": ""}

        # Local emergency always comes first
        if fallback_emergency(rows):
            JOBS[job_id]["summary"] = "local_emergency_triggered"
            return

        # Ask the model
        decision = ollama_decision(rows)
        logging.info(f"AI decision: {decision}")

        action          = decision.get("action", "none")
        severity        = decision.get("severity", "info")
        email_recipient = decision.get("email_recipient", "none")
        email_subject   = decision.get("email_subject", "")
        email_body      = decision.get("email_body", "")

        last = rows[-1]

        # ======================= HYBRID LOGIC =======================

        if action == "send_email":
            sent = False

            if email_recipient in ("user", "both") and EMAIL_TO_USER:
                sent |= send_email(email_subject, email_body, EMAIL_TO_USER)

            if email_recipient in ("emergency", "both") and EMAIL_TO_EMERGENCY:
                sent |= send_email(email_subject, email_body, EMAIL_TO_EMERGENCY)

            if sent:
                JOBS[job_id] = {"status": "done", "summary": json.dumps(decision), "error": ""}
                return

        # If model chose "none" or invalid JSON → send safe summary
        safe_summary = build_safe_summary(last)
        send_email("Rover Status: Normal Operation", safe_summary, EMAIL_TO_USER)

        JOBS[job_id] = {"status": "done", "summary": "safe_summary_sent", "error": ""}

    except Exception as e:
        logging.error(f"Job error: {e}")
        JOBS[job_id] = {"status": "error", "summary": "", "error": str(e)}

# ======================= API ENDPOINTS =======================

@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status": "ok", "ts": int(time.time())})

@app.route("/upload", methods=["POST"])
def upload():
    if API_KEY and request.headers.get("X-API-Key") != API_KEY:
        return jsonify({"error": "unauthorized"}), 401

    try:
        data = request.get_json(force=True)
    except:
        return jsonify({"error": "invalid_json"}), 400

    try:
        rows = normalize_packet(data)
    except Exception as e:
        return jsonify({"error": str(e)}), 400

    job_id = str(uuid.uuid4())
    JOBS[job_id] = {"status": "queued", "summary": "", "error": ""}

    threading.Thread(target=analyze_job, args=(job_id, rows), daemon=True).start()

    return jsonify({"status": "accepted", "job_id": job_id, "batch_size": len(rows)}), 202

@app.route("/jobs/<job_id>")
def job_status(job_id):
    return jsonify(JOBS.get(job_id, {"error": "unknown_job"}))

# ======================= MAIN =======================

if __name__ == "__main__":
    logging.info(f"Server running on 0.0.0.0:{PORT}")
    app.run(host="0.0.0.0", port=PORT)

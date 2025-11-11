import os, csv, time, json, requests, smtplib, threading, uuid
from email.mime.text import MIMEText
from flask import Flask, request, jsonify
from dotenv import load_dotenv

# ======================= CONFIGURATION =======================
load_dotenv()
API_KEY   = os.getenv("API_KEY","")
SMTP_HOST = os.getenv("SMTP_HOST","")
SMTP_PORT = int(os.getenv("SMTP_PORT","587"))
SMTP_USER = os.getenv("SMTP_USER","")
SMTP_PASS = os.getenv("SMTP_PASS","")
EMAIL_TO_USER      = os.getenv("EMAIL_TO_USER","")
EMAIL_TO_EMERGENCY = os.getenv("EMAIL_TO_EMERGENCY","")
OLLAMA_URL   = os.getenv("OLLAMA_URL","http://localhost:11434/api/generate")
OLLAMA_MODEL = os.getenv("OLLAMA_MODEL","deepseek-r1:7b")
CSV_FILE     = os.getenv("CSV_FILE","data.csv")

app = Flask(__name__)
JOBS = {}  # job_id -> {"status":"queued|running|done|error", "summary": "...", "error": ""}

# ======================= INITIALIZATION =======================
def ensure_csv_header():
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, "w", newline="", encoding="utf-8") as f:
            csv.writer(f).writerow(["timestamp","temperature","humidity","gas"])

def clear_csv():
    """Clears old data after each analysis."""
    with open(CSV_FILE, "w", newline="", encoding="utf-8") as f:
        csv.writer(f).writerow(["timestamp","temperature","humidity","gas"])
    print("[DATA] Cleared CSV after analysis")

# ======================= EMAIL FUNCTION =======================
def send_email(subject, body, to_addr):
    if not (SMTP_USER and SMTP_PASS and to_addr):
        print("Email not configured; skipping.")
        return
    msg = MIMEText(body)
    msg["Subject"] = subject
    msg["From"] = SMTP_USER
    msg["To"] = to_addr
    with smtplib.SMTP(SMTP_HOST, SMTP_PORT) as s:
        s.starttls()
        s.login(SMTP_USER, SMTP_PASS)
        s.sendmail(SMTP_USER, [to_addr], msg.as_string())
    print(f"[EMAIL] Sent '{subject}' to {to_addr}")

# ======================= AI PROMPT TEMPLATE =======================
PROMPT = """**Safety Summary: Mobile Automated Car Project**

Rules:
- If temperature > 70 °C → note overheating risk.
- If gas index > 1.0 → note gas hazard.
- Always give bullet points and an "Action Required" section (≤120 words total).
- Ignore unrelated contexts.

Data (timestamp, °C, %, gas):
{block}
"""

# ======================= OLLAMA SUMMARY =======================
def ollama_summary(rows):
    block = "\n".join([f"{r['timestamp']}, {r['temperature']}, {r['humidity']}, {r['gas']}"
                       for r in rows[-40:]])
    payload = {
        "model": OLLAMA_MODEL,
        "prompt": PROMPT.replace("{block}", block),
        "stream": False
    }
    try:
        res = requests.post(OLLAMA_URL, json=payload, timeout=180)
        try:
            js = res.json()
            return (js.get("response") or "").strip() or "No summary."
        except ValueError:
            text = res.text.splitlines()
            out = []
            for line in text:
                line = line.strip()
                if not line:
                    continue
                try:
                    part = json.loads(line)
                    if "response" in part:
                        out.append(part["response"])
                except Exception:
                    pass
            return "".join(out).strip() or f"Ollama unexpected response:\n{res.text[:500]}"
    except Exception as e:
        return f"Ollama error: {e}"

# ======================= DATA HANDLING =======================
def append_and_rules(packet):
    """Appends new readings to CSV and checks emergency conditions."""
    readings = []
    if isinstance(packet, dict) and all(k in packet for k in ("timestamp","temperature","humidity","gas")):
        readings = [packet]
    elif isinstance(packet, dict) and "records" in packet and isinstance(packet["records"], list):
        readings = packet["records"]
    else:
        raise ValueError("Bad JSON format: send either a single reading or {'records':[...]}")

    with open(CSV_FILE, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        for r in readings:
            w.writerow([r["timestamp"], r["temperature"], r["humidity"], r["gas"]])

    # Check last reading for emergency
    last = readings[-1]
    t = float(last["temperature"])
    g = float(last["gas"])
    h = float(last["humidity"])
    reasons = []
    if t > 70.0: reasons.append(f"temperature {t} °C > 70")
    if g > 1.0:  reasons.append(f"gas {g} > 1.0")

    if reasons:
        body = (f"Emergency detected at {last['timestamp']}\n"
                f"T={t} °C, H={h} %, G={g}\n" +
                "Reasons: " + "; ".join(reasons))
        send_email("Emergency Alert", body, EMAIL_TO_EMERGENCY)

    return readings

# ======================= BACKGROUND ANALYSIS =======================
def analyze_job(job_id, batch_rows):
    try:
        JOBS[job_id] = {"status": "running", "summary": "", "error": ""}

        rows = [{"timestamp": r["timestamp"],
                 "temperature": float(r["temperature"]),
                 "humidity": float(r["humidity"]),
                 "gas": float(r["gas"])} for r in batch_rows]

        print(f"[AI] Calling Ollama model '{OLLAMA_MODEL}' on batch of {len(rows)} rows...")
        summary = ollama_summary(rows)
        print("[AI] Summary:\n" + summary)

        send_email("Safety Summary", summary, EMAIL_TO_USER)
        clear_csv()  # <-- clear data after sending

        JOBS[job_id] = {"status": "done", "summary": summary, "error": ""}
    except Exception as e:
        print("[AI ERROR]", e)
        JOBS[job_id] = {"status": "error", "summary": "", "error": str(e)}

# ======================= API ENDPOINTS =======================
@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status":"ok","ts":int(time.time())})

@app.route("/upload", methods=["POST"])
def upload():
    if API_KEY and request.headers.get("X-API-Key") != API_KEY:
        return jsonify({"error":"unauthorized"}), 401

    try:
        data = request.get_json(force=True)
        batch = append_and_rules(data)  # append CSV + rule-based emergency (fast)

        # queue background AI analysis
        job_id = str(uuid.uuid4())
        JOBS[job_id] = {"status":"queued", "summary":"", "error":""}
        t = threading.Thread(target=analyze_job, args=(job_id, batch), daemon=True)
        t.start()

        return jsonify({"status":"accepted", "job_id": job_id, "batch_size": len(batch)}), 202
    except Exception as e:
        print("[ERROR] /upload failed:", e)
        return jsonify({"error": str(e)}), 400

@app.route("/jobs/<job_id>", methods=["GET"])
def job_status(job_id):
    job = JOBS.get(job_id)
    if not job:
        return jsonify({"error":"unknown job"}), 404
    return jsonify(job), 200

# ======================= MAIN =======================
if __name__ == "__main__":
    ensure_csv_header()
    app.run(host="0.0.0.0", port=5000)

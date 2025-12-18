from flask import Flask, request, jsonify
import mysql.connector
import time
import json

app = Flask(__name__)

# ================== SQL CONFIG ==================
db = mysql.connector.connect(
    host="localhost",
    user="root",
    password="Database100",
    database="homeguard" 
)

cursor = db.cursor()

# ================== INSERT FUNCTION ==================
def insert_reading(ts, temperature, humidity, pressure, gas_level):
    query = """
        INSERT INTO sensor_readings (timestamp, temperature, humidity, pressure, gas_level)
        VALUES (%s, %s, %s, %s, %s)
    """
    cursor.execute(query, (ts, temperature, humidity, pressure, gas_level))
    db.commit()

# ================== ENDPOINT ==================
@app.route("/upload", methods=["POST"])
def upload():
    try:
        data = request.get_json(force=True)

        # If STM sends a batch or one reading:
        if "records" in data:
            records = data["records"]
        else:
            records = [data]

        inserted = 0

        for r in records:
            ts         = r["timestamp"]
            temp       = float(r["temperature"])
            hum        = float(r["humidity"])
            pressure   = float(r.get("pressure", 0.0))      # default if STM doesn't send it
            gas_level  = float(r["gas"])

            insert_reading(ts, temp, hum, pressure, gas_level)
            inserted += 1

        return jsonify({"status": "ok", "inserted": inserted}), 200

    except Exception as e:
        print("UPLOAD ERROR:", e)
        return jsonify({"error": str(e)}), 400

@app.route("/health")
def health():
    return {"status": "ok", "ts": int(time.time())}

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)

import os
import time
import json
import logging
import requests
import mysql.connector


# CONFIG


ANALYZER_IP = "100.80.75.92"
URL = f"http://{ANALYZER_IP}:5000/upload"

MYSQL_CONFIG = {
    "host": "localhost",
    "database": "homeguard",
    "user": "root",
    "password": "Database100",
}

POLL_INTERVAL = 150        # seconds between DB checks
CLEANUP_INTERVAL = 3600  # seconds between cleanup runs (1 hour)

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CHECKPOINT_FILE = os.path.join(BASE_DIR, "last_sent_id.txt")
ENV_FILE = os.path.join(BASE_DIR, ".env")
LOG_FILE = os.path.join(BASE_DIR, "forwarder.log")



# LOGGING SETUP


logging.basicConfig(
    filename=LOG_FILE,
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)



# ENV + API KEY

def load_env_file(filepath=ENV_FILE):
    if not os.path.exists(filepath):
        logging.warning(f".env file not found at {filepath}")
        return
    with open(filepath) as f:
        for line in f:
            if line.strip() and not line.startswith("#"):
                key, value = line.strip().split("=", 1)
                os.environ[key] = value


load_env_file()
API_KEY = os.getenv("API_KEY")
if not API_KEY:
    logging.error("API_KEY not found in environment. Check .env file.")
HEADERS = {"X-API-Key": API_KEY, "Content-Type": "application/json"}



# DB UTILITIES


def get_last_sent_id():
    if not os.path.exists(CHECKPOINT_FILE):
        return 0
    try:
        with open(CHECKPOINT_FILE, "r") as f:
            return int(f.read().strip())
    except Exception as e:
        logging.error(f"Error reading checkpoint file: {e}")
        return 0


def save_last_sent_id(last_id: int):
    try:
        with open(CHECKPOINT_FILE, "w") as f:
            f.write(str(last_id))
    except Exception as e:
        logging.error(f"Error writing checkpoint file: {e}")


def fetch_new_records(last_sent_id: int):
    try:
        db = mysql.connector.connect(**MYSQL_CONFIG)
        cursor = db.cursor(dictionary=True)

        cursor.execute(
            """
            SELECT id, timestamp, gas_level, temperature, humidity, pressure
            FROM sensor_readings
            WHERE id > %s
            ORDER BY id ASC
            """,
            (last_sent_id,),
        )

        rows = cursor.fetchall()
        cursor.close()
        db.close()
        return rows

    except mysql.connector.Error as e:
        logging.error(f"MySQL error while fetching records: {e}")
        return []


def cleanup_old_rows():
    try:
        db = mysql.connector.connect(**MYSQL_CONFIG)
        cursor = db.cursor()
        cursor.execute(
            """
            DELETE FROM sensor_readings
            WHERE timestamp < NOW() - INTERVAL 48 HOUR
            """
        )
        deleted = cursor.rowcount
        db.commit()
        cursor.close()
        db.close()
        logging.info(f"Cleanup complete, removed {deleted} old rows.")
    except mysql.connector.Error as e:
        logging.error(f"MySQL error during cleanup: {e}")



# PACKET + SENDING


def build_packet(rows):
    return {
        "records": [
            {
                "timestamp": str(r["timestamp"]),
                "temperature": r["temperature"],
                "humidity": r["humidity"],
                "gas": r["gas_level"],
                "pressure": r["pressure"],
            }
            for r in rows
        ]
    }


def send_packet(packet):
    if not API_KEY:
        logging.error("Cannot send data: API_KEY is missing.")
        return False

    try:
        resp = requests.post(URL, json=packet, headers=HEADERS, timeout=20)
        logging.info(f"Sent {len(packet['records'])} records, status={resp.status_code}")
        if resp.status_code != 200:
            logging.warning(f"Server response: {resp.text}")
        return resp.status_code == 200
    except requests.exceptions.RequestException as e:
        logging.error(f"Request error while sending packet: {e}")
        return False


# MAIN LOOP

def main_loop():
    logging.info("Forwarder service starting...")
    last_cleanup = time.time()

    while True:
        try:
            last_sent_id = get_last_sent_id()
            rows = fetch_new_records(last_sent_id)

            if rows:
                packet = build_packet(rows)
                logging.info(f"New rows to send: {len(rows)}")

                if send_packet(packet):
                    new_last_id = rows[-1]["id"]
                    save_last_sent_id(new_last_id)
                    logging.info(f"Checkpoint updated to ID {new_last_id}")
                else:
                    logging.warning("Send failed, will retry later.")

            # periodic cleanup (48h retention)
            now = time.time()
            if now - last_cleanup > CLEANUP_INTERVAL:
                cleanup_old_rows()
                last_cleanup = now

        except Exception as e:
            logging.error(f"Unexpected error in main loop: {e}")

        time.sleep(POLL_INTERVAL)


if __name__ == "__main__":
    main_loop()

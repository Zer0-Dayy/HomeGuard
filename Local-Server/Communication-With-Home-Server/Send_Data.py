import requests, time, os

ANALYZER_IP = "100.80.75.92" 
URL = f"http://{ANALYZER_IP}:5000/upload"

def load_env_file(filepath="./.env"):
    with open(filepath) as f:
        for line in f:
            if line.strip() and not line.startswith("#"):
                key, value = line.strip().split("=", 1)
                os.environ[key] = value


load_env_file()

API_KEY = os.getenv("API_KEY")
print(API_KEY)
headers = {"X-API-Key": API_KEY, "Content-Type": "application/json"}

# Example batch payload (preferred)
packet = {
  "records": [
    {"timestamp":"2025-10-23 18:01:00","temperature":25.4,"humidity":60.2,"gas":0.12},
    {"timestamp":"2025-10-23 18:02:00","temperature":27.1,"humidity":58.0,"gas":0.20},
    {"timestamp":"2025-10-23 18:03:00","temperature":72.0,"humidity":55.0,"gas":1.40}
  ]
}

r = requests.post(URL, json=packet, headers=headers, timeout=30)
print(r.status_code, r.text)

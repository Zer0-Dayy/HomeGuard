import pandas as pd
import mysql.connector
from mysql.connector import Error

MYSQL_CONFIG = {
    'host': 'localhost',
    'database': 'homeguard',
    'user': 'root',
    'password': 'Database100'
}

def get_mysql_connection():
    try:
        db = mysql.connector.connect(**MYSQL_CONFIG)
        if db.is_connected():
            return db
    except Error as e:
        print(f"MySQL Connection Error: {e}")
    return None

def get_latest_readings():
    db = get_mysql_connection()
    if not db:
        return None

    try:
        cursor = db.cursor(dictionary=True)
        cursor.execute("""
            SELECT id, timestamp, gas_level, temperature, humidity, pressure, alert_status
            FROM sensor_readings
            ORDER BY timestamp DESC
            LIMIT 1
        """)
        result = cursor.fetchone()
        cursor.close()
        db.close()
        return result
    except Error as e:
        print(f"Fetch Error: {e}")
        return None

def get_historical_data(hours=48):
    db = get_mysql_connection()
    if not db:
        return None

    try:
        cursor = db.cursor(dictionary=True)
        cursor.execute("""
            SELECT timestamp, gas_level, temperature, humidity, pressure, alert_status
            FROM sensor_readings
            WHERE timestamp >= NOW() - INTERVAL %s HOUR
            ORDER BY timestamp ASC
        """, (hours,))
        rows = cursor.fetchall()
        cursor.close()
        db.close()

        # No data → empty DataFrame instead of None
        if not rows:
            return pd.DataFrame()

        df = pd.DataFrame(rows)
        df["timestamp"] = pd.to_datetime(df["timestamp"])
        return df

    except Error as e:
        print(f"Historical Data Error: {e}")
        return None

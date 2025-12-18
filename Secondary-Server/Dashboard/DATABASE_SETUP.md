# Database Setup Guide

## Quick Setup Steps

When your friend gives you the SQL database credentials, follow these simple steps:

### Step 1: Get Database Information
Ask your friend for:
- **Host**: The MySQL server address (e.g., `localhost`, `192.168.1.100`, or an IP address)
- **Database Name**: The name of the database (e.g., `homecare_db`)
- **Username**: MySQL username (e.g., `root`)
- **Password**: MySQL password

### Step 2: Update `database.py`

1. Open `database.py` file
2. Find the `MYSQL_CONFIG` section (around line 23)
3. Replace the placeholder values with your actual database credentials:

```python
MYSQL_CONFIG = {
    'host': 'YOUR_HOST_HERE',        # e.g., 'localhost' or '192.168.1.100'
    'database': 'YOUR_DATABASE_NAME', # e.g., 'homecare_db'
    'user': 'YOUR_USERNAME',          # e.g., 'root'
    'password': 'YOUR_PASSWORD'       # Your actual password
}
```

### Step 3: Switch to Real Database

1. In `database.py`, find this line (around line 19):
   ```python
   USE_MOCK_DATA = True
   ```

2. Change it to:
   ```python
   USE_MOCK_DATA = False
   ```

### Step 4: Verify Database Schema

Make sure your friend's database has a table called `sensor_readings` with these columns:
- `id` (INT, AUTO_INCREMENT, PRIMARY KEY)
- `timestamp` (DATETIME)
- `gas_level` (FLOAT)
- `temperature` (FLOAT)
- `humidity` (FLOAT)
- `pressure` (FLOAT)
- `alert_status` (VARCHAR)

If the table doesn't exist, your friend can create it using the SQL schema in `database.py` (lines 240-253).

### Step 5: Test the Connection

1. Make sure `mysql-connector-python` is installed:
   ```bash
   pip install mysql-connector-python
   ```

2. Run your Streamlit app:
   ```bash
   streamlit run app.py
   ```

3. If you see data from your database, it's working! ✅
4. If you see errors, check:
   - Database credentials are correct
   - MySQL server is running
   - Database name and table name are correct
   - Your computer can reach the MySQL server (network/firewall)

### Troubleshooting

**Error: "Can't connect to MySQL server"**
- Check if the host address is correct
- Verify MySQL server is running
- Check firewall/network settings

**Error: "Access denied"**
- Verify username and password are correct
- Check if the user has permission to access the database

**Error: "Table doesn't exist"**
- Verify the table name is `sensor_readings`
- Check if the database name is correct
- Ask your friend to create the table using the schema in `database.py`

**No data showing**
- Check if there's actual data in the `sensor_readings` table
- Verify the timestamp column has recent data

That's it! Once you switch `USE_MOCK_DATA = False`, the dashboard will automatically use real data from your MySQL database.


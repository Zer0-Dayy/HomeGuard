# HomeGuard Monitoring Dashboard 🏠

A futuristic IoT sensor dashboard built with Streamlit for real-time environmental monitoring using STM32 microcontroller data.

## 🌟 Features

- **Real-time Monitoring**: Display current readings from multiple sensors
- **Dark/Light Theme Toggle**: Switch between futuristic dark and clean light themes
- **Interactive Charts**: Visualize historical data with Plotly charts
- **Smart Alerts**: Automatic threshold-based warnings for hazardous conditions
- **Responsive Design**: Modern, clean UI with smooth animations
- **MySQL Ready**: Easy switch from mock data to real database

## 🔧 Sensors Monitored

- **MQ-2 Gas Sensor**: Air quality detection (clean/moderate/hazardous)
- **BME280 Temperature**: Celsius readings with high-temperature alerts
- **BME280 Humidity**: Relative humidity monitoring
- **BME280 Pressure**: Atmospheric pressure tracking

## 🚀 Quick Start

### 1. Install Dependencies

```bash
pip install -r requirements.txt
```

### 2. Run the Dashboard

```bash
streamlit run app.py
```

The dashboard will open in your browser at `http://localhost:8501`

## 📊 Current Mode: Mock Data

The dashboard is currently running with **mock data** to demonstrate functionality. Random realistic sensor values are generated to simulate real-world conditions.

## 🔄 Switching to Real MySQL Database

To connect to your actual MySQL database:

1. Open `database.py`
2. Set `USE_MOCK_DATA = False`
3. Configure `MYSQL_CONFIG` with your database credentials
4. Uncomment the MySQL functions
5. Create the database schema (SQL provided in `database.py`)
6. Restart the dashboard

Detailed instructions are provided in `database.py`.

## 📁 Project Structure

```
streamlit/
├── app.py                 # Main dashboard application
├── database.py           # Database connection & mock data
├── requirements.txt      # Python dependencies
├── .streamlit/
│   └── config.toml      # Streamlit configuration
└── README.md            # This file
```

## ⚠️ Alert Thresholds

- **Gas Level**:
  - Clean Air: 0.8 - 1.0
  - Moderate: 0.3 - 0.8
  - Hazardous: 0.0 - 0.3
  
- **Temperature**: Alert if > 30°C
- **Humidity**: Info if < 30% or > 70%

## 🎨 Theme Options

Toggle between:
- **Dark Theme**: Futuristic cyberpunk aesthetic (default)
- **Light Theme**: Clean, professional appearance

## 🛠️ Technologies Used

- **Streamlit**: Web framework
- **Plotly**: Interactive charts
- **Pandas**: Data manipulation
- **MySQL**: Database (production ready)

## 📝 Database Schema

```sql
CREATE TABLE sensor_readings (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp DATETIME NOT NULL,
    gas_level FLOAT NOT NULL,
    temperature FLOAT NOT NULL,
    humidity FLOAT NOT NULL,
    pressure FLOAT NOT NULL,
    alert_status VARCHAR(50) DEFAULT 'Normal'
);
```

## 🔄 Auto-Refresh

The dashboard includes:
- Manual refresh button
- Optional 10-second auto-refresh (commented out in code)

To enable auto-refresh, uncomment the last two lines in `app.py`.

## 👥 Team Integration

Your teammate can easily integrate real MySQL data by following the instructions in `database.py`. The mock data structure matches the production schema exactly, ensuring seamless transition.

## 📧 Support

For issues or questions, refer to the comments in `database.py` for database integration guidance.

---

**Built with ❤️ for HomeGuard Environmental Monitoring**

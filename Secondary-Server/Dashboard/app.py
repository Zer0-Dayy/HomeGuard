import streamlit as st
import pandas as pd
import plotly.graph_objects as go
from datetime import datetime, timedelta
from database import get_latest_readings, get_historical_data


# Page configuration
st.set_page_config(
    page_title="HomeGuard Monitoring Dashboard",
    page_icon="🏠",
    layout="wide",
    initial_sidebar_state="collapsed"
)

# Initialize session state
if 'theme' not in st.session_state:
    st.session_state.theme = 'dark'

# Custom CSS matching shadcn/ui design
def apply_custom_css(theme):
    # Zinc Palette (Shadcn Default)
    if theme == 'dark':
        bg_color = "#09090b"
        text_color = "#fafafa"
        card_bg = "#18181b"  # Slightly lighter than bg for contrast in dark mode
        border_color = "#27272a"
        muted_color = "#a1a1aa"
        primary_color = "#fafafa"
        secondary_bg = "#27272a"
        
        # Chart colors (Dark)
        chart_colors = ["#2eb88a", "#e23670", "#e88c30", "#2662d9", "#af57db"]
    else:
        bg_color = "#ffffff"
        text_color = "#09090b"
        card_bg = "#ffffff"
        border_color = "#e4e4e7"
        muted_color = "#71717a"
        primary_color = "#18181b"
        secondary_bg = "#f4f4f5"
        
        # Chart colors (Light)
        chart_colors = ["#0f766e", "#be185d", "#c2410c", "#1d4ed8", "#7e22ce"]
    
    st.markdown(f"""
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap');

        :root {{
            /* Custom Theme Variables */
            --background: {bg_color};
            --foreground: {text_color};
            --card: {card_bg};
            --card-foreground: {text_color};
            --popover: {card_bg};
            --popover-foreground: {text_color};
            --primary: {primary_color};
            --primary-foreground: {bg_color};
            --muted: {secondary_bg};
            --muted-foreground: {muted_color};
            --accent: {secondary_bg};
            --accent-foreground: {text_color};
            --border: {border_color};
            --input: {border_color};
            --ring: {muted_color};
            --radius: 0.5rem;

        }}

        /* Base App Styles */
        .stApp {{
            background-color: var(--background);
            color: var(--foreground);
            font-family: 'Inter', sans-serif;
        }}

        /* Force background color on the body for popovers that attach to body */
        body {{
            background-color: var(--background);
            color: var(--foreground);
        }}

        /* Fix text visibility in light mode for standard Streamlit elements */
        p, h1, h2, h3, h4, h5, h6, li, span, div {{
            color: var(--foreground);
        }}

        /* Metric Cards (shadcn card style) */
        .metric-card {{
            background-color: var(--card);
            color: var(--card-foreground);
            border-radius: var(--radius);
            border: 1px solid var(--border);
            box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
            padding: 1.5rem;
            height: 100%;
            transition: all 0.2s ease-in-out;
        }}
        
        .metric-card:hover {{
            transform: translateY(-2px);
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
        }}
        
        .metric-header {{
            display: flex;
            flex-direction: row;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 0.5rem;
        }}
        
        .metric-label {{
            font-size: 0.875rem;
            font-weight: 500;
            color: var(--muted-foreground);
        }}
        
        .metric-value {{
            font-size: 1.5rem;
            font-weight: 700;
            color: var(--foreground);
            margin-top: 0.25rem;
        }}

        .metric-footer {{
            margin-top: 1rem;
            font-size: 0.75rem;
            color: var(--muted-foreground);
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }}

        .trend-badge {{
            display: inline-flex;
            align-items: center;
            border-radius: 0.375rem;
            padding: 0.125rem 0.5rem;
            font-size: 0.75rem;
            font-weight: 500;
            line-height: 1rem;
        }}

        .trend-up {{
            color: #22c55e;
            background-color: rgba(34, 197, 94, 0.1);
        }}
        
        .trend-down {{
            color: #ef4444;
            background-color: rgba(239, 68, 68, 0.1);
        }}

        /* Header Styles */
        .dashboard-header {{
            padding: 2rem 0 1rem 0;
            border-bottom: 1px solid var(--border);
            margin-bottom: 2rem;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }}
        
        .dashboard-title {{
            font-size: 1.875rem;
            font-weight: 700;
            letter-spacing: -0.025em;
            color: var(--foreground);
            margin: 0;
        }}

        /* Alert Banners */
        .alert-banner {{
            border-radius: var(--radius);
            padding: 1rem;
            margin-bottom: 1rem;
            border: 1px solid transparent;
            display: flex;
            align-items: center;
            gap: 0.75rem;
            font-size: 0.875rem;
            font-weight: 500;
        }}
        
        .alert-danger {{
            border-color: rgba(239, 68, 68, 0.5);
            background-color: rgba(239, 68, 68, 0.1);
            color: #ef4444;
        }}
        
        .alert-warning {{
            border-color: rgba(245, 158, 11, 0.5);
            background-color: rgba(245, 158, 11, 0.1);
            color: #f59e0b;
        }}
        
        .alert-success {{
            border-color: rgba(34, 197, 94, 0.5);
            background-color: rgba(34, 197, 94, 0.1);
            color: #22c55e;
        }}

        /* Streamlit Elements Overrides */
        div[data-testid="stButton"] > button {{
            background-color: var(--card) !important;
            color: var(--foreground) !important;
            border: 1px solid var(--border) !important;
            border-radius: var(--radius) !important;
            font-weight: 500 !important;
            padding: 0.5rem 1rem !important;
            transition: all 0.2s ease !important;
            box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.05) !important;
        }}
        
        div[data-testid="stButton"] > button:hover {{
            background-color: var(--muted) !important;
            color: var(--foreground) !important;
            border-color: var(--border) !important;
            transform: translateY(-1px);
            box-shadow: 0 2px 4px 0 rgba(0, 0, 0, 0.1) !important;
        }}

        div[data-testid="stButton"] > button:active {{
            background-color: var(--muted) !important;
            color: var(--foreground) !important;
            transform: translateY(0);
        }}
        
        div[data-testid="stButton"] > button:focus {{
            outline: 2px solid var(--ring) !important;
            outline-offset: 2px !important;
        }}

        /* Hide Streamlit branding */
        #MainMenu {{visibility: hidden;}}
        footer {{visibility: hidden;}}
        header {{visibility: hidden;}}
        
        /* Tabs overrides */
        .stTabs [data-baseweb="tab-list"] {{
            gap: 8px;
            background-color: transparent;
        }}
        
        .stTabs [data-baseweb="tab"] {{
            height: auto;
            padding: 8px 16px;
            border-radius: var(--radius);
            background-color: transparent;
            color: var(--muted-foreground);
            border: 1px solid transparent;
        }}
        
        .stTabs [data-baseweb="tab"]:hover {{
            color: var(--foreground);
            background-color: var(--muted);
        }}
        
        .stTabs [aria-selected="true"] {{
            background-color: var(--card) !important;
            color: var(--foreground) !important;
            border: 1px solid var(--border) !important;
            font-weight: 600;
            box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
        }}

        /* Selectbox override */
        div[data-baseweb="select"] > div {{
            background-color: var(--card) !important;
            border-color: var(--border) !important;
            color: var(--foreground) !important;
            border-radius: var(--radius) !important;
        }}
        
        div[data-baseweb="select"] span {{
            color: var(--foreground) !important;
        }}
        
        /* Selectbox dropdown menu */
        div[data-baseweb="popover"],
        div[data-baseweb="popover"] > div,
        div[data-baseweb="popover"] > div > div,
        div[data-baseweb="popover"] > div > div > div,
        div[data-baseweb="menu"],
        div[data-baseweb="menu"] > div,
        div[data-baseweb="menu"] > div > div,
        ul[role="listbox"],
        ul[role="listbox"] > li,
        ul[role="listbox"] > li > div {{
            background-color: {card_bg} !important;
            color: {text_color} !important;
            border-color: {border_color} !important;
        }}

        /* Ensure the border radius and shadow are only on the main container */
        div[data-baseweb="popover"] {{
            border: 1px solid {border_color} !important;
            border-radius: var(--radius) !important;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05) !important;
        }}

        /* Specific fix for the list container */
        ul[role="listbox"] {{
            padding: 0.25rem !important;
            background-color: {card_bg} !important;
        }}

        /* Fix for items - target all possible nested structures */
        li[role="option"],
        li[role="option"] > div,
        li[role="option"] > span,
        li[role="option"] > div > span {{
            background-color: transparent !important;
            color: {text_color} !important;
        }}

        li[role="option"]:hover,
        li[role="option"]:hover > div,
        li[role="option"]:hover > span,
        li[role="option"][aria-selected="true"],
        li[role="option"][aria-selected="true"] > div,
        li[role="option"][aria-selected="true"] > span {{
            background-color: {secondary_bg} !important;
            color: {text_color} !important;
        }}

        /* Force all text inside popover to be the correct color */
        div[data-baseweb="popover"] *,
        div[data-baseweb="popover"] * *,
        div[data-baseweb="popover"] * * * {{
            color: {text_color} !important;
        }}
        
        /* Target any divs inside the popover */
        div[data-baseweb="popover"] div,
        div[data-baseweb="popover"] div div,
        div[data-baseweb="popover"] div div div {{
            background-color: {card_bg} !important;
            color: {text_color} !important;
        }}
        
        /* Target spans and text elements */
        div[data-baseweb="popover"] span,
        div[data-baseweb="popover"] p,
        div[data-baseweb="popover"] label {{
            color: {text_color} !important;
        }}
        
        /* Target Streamlit-specific classes inside dropdowns */
        div[data-baseweb="popover"] .stVerticalBlock,
        div[data-baseweb="popover"] [class*="st-emotion"],
        div[data-baseweb="popover"] [class*="st-emotion"] * {{
            background-color: {card_bg} !important;
            color: {text_color} !important;
        }}
        
        /* Selectbox arrow icon */
        svg[data-baseweb="select-arrow-icon"] {{
            color: {muted_color} !important;
        }}
        
        
        /* Expander override */
        .streamlit-expanderHeader {{
            background-color: var(--card);
            color: var(--foreground);
            border-radius: var(--radius);
        }}
        
        /* Spacing between sections */
        .metric-cards-section {{
            margin-bottom: 2.5rem;
        }}
        
        .chart-section {{
            margin-top: 2.5rem;
        }}
        
        .chart-header {{
            margin-bottom: 1.5rem;
        }}
        
        /* Spacing for selectbox */
        div[data-baseweb="select"] {{
            margin-top: 0.5rem;
        }}
        
    </style>
    <script>
        (function() {{
            const cardBg = '{card_bg}';
            const textColor = '{text_color}';
            const borderColor = '{border_color}';
            
            function forceDropdownStyles() {{
                // Force style all popover elements
                const popovers = document.querySelectorAll('[data-baseweb="popover"]');
                popovers.forEach(popover => {{
                    popover.style.setProperty('background-color', cardBg, 'important');
                    popover.style.setProperty('color', textColor, 'important');
                    popover.style.setProperty('border-color', borderColor, 'important');
                    
                    // Force style all children
                    const allChildren = popover.querySelectorAll('*');
                    allChildren.forEach(child => {{
                        if (child.tagName === 'LI' || child.tagName === 'UL' || child.tagName === 'DIV') {{
                            child.style.setProperty('background-color', cardBg, 'important');
                        }}
                        child.style.setProperty('color', textColor, 'important');
                    }});
                }});
                
                // Force style list items
                const listItems = document.querySelectorAll('[role="option"]');
                listItems.forEach(item => {{
                    item.style.setProperty('color', textColor, 'important');
                    item.style.setProperty('background-color', 'transparent', 'important');
                    
                    // Style nested elements
                    const nested = item.querySelectorAll('*');
                    nested.forEach(el => {{
                        el.style.setProperty('color', textColor, 'important');
                    }});
                }});
                
                // Force style listboxes
                const listboxes = document.querySelectorAll('[role="listbox"]');
                listboxes.forEach(lb => {{
                    lb.style.setProperty('background-color', cardBg, 'important');
                    lb.style.setProperty('color', textColor, 'important');
                }});
                
                // Force style Streamlit-specific classes
                const stBlocks = document.querySelectorAll('[data-baseweb="popover"] .stVerticalBlock, [data-baseweb="popover"] [class*="st-emotion"]');
                stBlocks.forEach(block => {{
                    block.style.setProperty('background-color', cardBg, 'important');
                    block.style.setProperty('color', textColor, 'important');
                    
                    const stChildren = block.querySelectorAll('*');
                    stChildren.forEach(child => {{
                        child.style.setProperty('color', textColor, 'important');
                        if (child.tagName === 'DIV' || child.tagName === 'SPAN') {{
                            child.style.setProperty('background-color', cardBg, 'important');
                        }}
                    }});
                }});
            }}
            
            // Run immediately
            forceDropdownStyles();
            
            // Watch for DOM changes
            const observer = new MutationObserver(forceDropdownStyles);
            observer.observe(document.body, {{ childList: true, subtree: true }});
            
            // Run on click (when dropdown opens)
            document.addEventListener('click', () => {{
                setTimeout(forceDropdownStyles, 10);
                setTimeout(forceDropdownStyles, 100);
                setTimeout(forceDropdownStyles, 300);
            }});
        }})();
    </script>
    """, unsafe_allow_html=True)
        
    return chart_colors, muted_color

# Apply theme
chart_colors, muted_color = apply_custom_css(st.session_state.theme)

# Header with theme toggle
header_col1, header_col2 = st.columns([6, 1])
with header_col1:
    st.markdown('<div class="dashboard-header"><h1 class="dashboard-title">🏠 HomeGuard Monitoring Dashboard</h1></div>', unsafe_allow_html=True)
with header_col2:
    if st.button("☀️" if st.session_state.theme == 'dark' else "🌙", key="theme_toggle", help="Toggle Theme"):
        st.session_state.theme = 'light' if st.session_state.theme == 'dark' else 'dark'
        st.rerun()

# Get latest sensor readings
latest_data = get_latest_readings()
historical_df = get_historical_data(hours=48)

# Handle database connection errors
if latest_data is None:
    st.error("⚠️ **Database Connection Error**: Could not fetch latest readings. Please check your database connection.")
    st.stop()

if historical_df is None or historical_df.empty:
    st.error("⚠️ **Database Error**: Could not fetch historical data. Please check your database connection and ensure data exists.")
    st.stop()

# Calculate trends
def calculate_trend(current, historical_values):
    if len(historical_values) > 0:
        avg_previous = sum(historical_values[-10:]) / len(historical_values[-10:])
        change = ((current - avg_previous) / avg_previous) * 100
        return change
    return 0

gas_trend = calculate_trend(latest_data['gas_level'], historical_df['gas_level'].tolist())
temp_trend = calculate_trend(latest_data['temperature'], historical_df['temperature'].tolist())
humidity_trend = calculate_trend(latest_data['humidity'], historical_df['humidity'].tolist())
pressure_trend = calculate_trend(latest_data['pressure'], historical_df['pressure'].tolist())

# Alert System
alerts = []
if latest_data['gas_level'] <= 0.3:
    alerts.append(("danger", "🚨 HAZARDOUS GAS DETECTED! Immediate ventilation required!"))
elif latest_data['gas_level'] <= 0.8:
    alerts.append(("warning", "⚠️ MODERATE GAS LEVEL - Monitor air quality"))
else:
    alerts.append(("success", "✓ Air quality is optimal"))

if latest_data['temperature'] > 30:
    alerts.append(("warning", "🌡️ HIGH TEMPERATURE - Room temperature elevated"))

# Display alerts
for alert_type, message in alerts:
    st.markdown(f'<div class="alert-banner alert-{alert_type}">{message}</div>', unsafe_allow_html=True)

# Metric cards section
st.markdown('<div class="metric-cards-section">', unsafe_allow_html=True)
col1, col2, col3, col4 = st.columns(4)

def metric_card(title, value, trend, icon, description):
    trend_icon = "↗" if trend >= 0 else "↘"
    trend_class = "trend-up" if trend >= 0 else "trend-down"
    
    return f"""
    <div class="metric-card">
        <div class="metric-header">
            <span class="metric-label">{title}</span>
            {icon}
        </div>
        <div class="metric-value">{value}</div>
        <div class="metric-footer">
            <span class="trend-badge {trend_class}">
                {trend_icon} {abs(trend):.1f}%
            </span>
            <span>{description}</span>
        </div>
    </div>
    """

with col1:
    st.markdown(metric_card("Gas Level (MQ-2)", f"{latest_data['gas_level']:.2f}", gas_trend, "☁️", "vs last hour"), unsafe_allow_html=True)

with col2:
    st.markdown(metric_card("Temperature", f"{latest_data['temperature']:.1f}°C", temp_trend, "🌡️", "vs last hour"), unsafe_allow_html=True)

with col3:
    st.markdown(metric_card("Humidity", f"{latest_data['humidity']:.1f}%", humidity_trend, "💧", "vs last hour"), unsafe_allow_html=True)

with col4:
    st.markdown(metric_card("Pressure", f"{latest_data['pressure']:.1f} hPa", pressure_trend, "⏲️", "vs last hour"), unsafe_allow_html=True)

st.markdown('</div>', unsafe_allow_html=True)

# Historical data chart
st.markdown('<div class="chart-section">', unsafe_allow_html=True)
chart_col1, chart_col2 = st.columns([3, 1])
with chart_col1:
    st.markdown('<div class="chart-header">', unsafe_allow_html=True)
    st.markdown('<h3 style="margin: 0 0 0.5rem 0; font-size: 1.25rem; font-weight: 600;">Sensor Readings Over Time</h3>', unsafe_allow_html=True)
    st.markdown('<p style="margin: 0; color: var(--muted-foreground); font-size: 0.875rem;">Real-time environmental monitoring data</p>', unsafe_allow_html=True)
    st.markdown('</div>', unsafe_allow_html=True)

with chart_col2:
    st.markdown('<div style="margin-top: 0.5rem;">', unsafe_allow_html=True)
    time_options = {"Last 6 hours": 6, "Last 12 hours": 12, "Last 24 hours": 24}
    selected_time = st.selectbox("Time Range", list(time_options.keys()), key="time_range_select", label_visibility="collapsed")
    hours = time_options[selected_time]
    st.markdown('</div>', unsafe_allow_html=True)

# Filter data based on time range
cutoff_time = datetime.now() - timedelta(hours=hours)
filtered_df = historical_df[historical_df['timestamp'] >= cutoff_time].copy().sort_values('timestamp')

# Create tabs for different views
st.markdown('<div style="margin-top: 1.5rem;">', unsafe_allow_html=True)
tab1, tab2, tab3, tab4 = st.tabs(["Temperature", "Humidity", "Pressure", "Gas Level"])

def create_chart(df, x_col, y_col, color_idx, unit):
    fig = go.Figure()
    color = chart_colors[color_idx]
    fig.add_trace(go.Scatter(
        x=df[x_col],
        y=df[y_col],
        mode='lines',
        line=dict(color=color, width=2),
        fill='tozeroy',
        fillcolor=f"rgba({int(color[1:3], 16)}, {int(color[3:5], 16)}, {int(color[5:7], 16)}, 0.1)",
        hovertemplate=f'%{{y:.1f}}{unit}<extra></extra>'
    ))
    fig.update_layout(
        xaxis=dict(showgrid=False, showline=False, showticklabels=True, tickfont=dict(color=muted_color, size=10)),
        yaxis=dict(showgrid=True, gridcolor='rgba(128, 128, 128, 0.1)', showline=False, showticklabels=True, tickfont=dict(color=muted_color, size=10)),
        hovermode='x unified',
        margin=dict(l=10, r=10, t=10, b=10),
        height=300,
        paper_bgcolor='rgba(0,0,0,0)',
        plot_bgcolor='rgba(0,0,0,0)',
        showlegend=False
    )
    return fig

with tab1:
    fig = create_chart(filtered_df, 'timestamp', 'temperature', 0, '°C')
    fig.add_hline(y=30, line_dash="dash", line_color="rgba(239, 68, 68, 0.5)")
    st.plotly_chart(fig, width="stretch")

with tab2:
    fig = create_chart(filtered_df, 'timestamp', 'humidity', 1, '%')
    st.plotly_chart(fig, width="stretch")

with tab3:
    fig = create_chart(filtered_df, 'timestamp', 'pressure', 2, ' hPa')
    st.plotly_chart(fig, width="stretch")

with tab4:
    fig = create_chart(filtered_df, 'timestamp', 'gas_level', 3, '')
    fig.add_hrect(y0=0, y1=0.3, fillcolor="rgba(239, 68, 68, 0.1)", line_width=0)
    fig.add_hrect(y0=0.3, y1=0.8, fillcolor="rgba(249, 115, 22, 0.1)", line_width=0)
    fig.add_hrect(y0=0.8, y1=1.0, fillcolor="rgba(34, 197, 94, 0.1)", line_width=0)
    st.plotly_chart(fig, width="stretch")

st.markdown('</div>', unsafe_allow_html=True)
st.markdown('</div>', unsafe_allow_html=True)

# Footer
st.markdown("<br>", unsafe_allow_html=True)
footer_col1, footer_col2, footer_col3 = st.columns([2, 1, 2])
with footer_col2:
    if st.button("🔄 Refresh Data", width="stretch"):
        st.rerun()

# Format timestamp (handle both datetime objects and strings)
try:
    if isinstance(latest_data['timestamp'], str):
        from datetime import datetime as dt
        timestamp = dt.strptime(latest_data['timestamp'], '%Y-%m-%d %H:%M:%S')
    else:
        timestamp = latest_data['timestamp']
    timestamp_str = timestamp.strftime('%B %d, %Y at %H:%M:%S')
except Exception as e:
    timestamp_str = "Unknown"

st.markdown(f"""
<div style="text-align: center; padding: 1.5rem; color: var(--muted-foreground); font-size: 0.875rem;">
    Last updated: {timestamp_str}
</div>
""", unsafe_allow_html=True)

import serial
import threading
import time
import re
from flask import Flask, jsonify
from flask_cors import CORS
import boto3 # For AWS S3
import json # For S3 data
import datetime # For S3 filenames
from botocore.exceptions import NoCredentialsError, PartialCredentialsError # For AWS errors

import argparse

# --- Configuration ---
SERIAL_PORT = 'COM3'  # Adjust to your serial port
BAUD_RATE = 115200    # Match the baud rate of your Icarus device

# --- AWS S3 Configuration ---
# IMPORTANT: Replace with your actual S3 bucket name and AWS region
AWS_S3_BUCKET = 'gt-stinger-bucket'
AWS_S3_REGION = 'us-east-2' # e.g., 'us-east-1'

# Initialize S3 client
# Ensure your AWS credentials are configured in your environment
# (e.g., via `aws configure` CLI command, or environment variables AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, AWS_SESSION_TOKEN)
s3_client = boto3.client('s3', region_name=AWS_S3_REGION)

S3_BUS_TYPE_PREFIX = "default_bus_type"

# --- Globals to store the latest sensor data ---
# "raw_lines" is removed from here, so it won't be part of the local API response
latest_data = {
    "timestamp": None,
    "gps_fix_valid": False,
    "latitude": None,
    "longitude": None,
    "altitude": None,
    "speed": None,
    "bearing": None,
    "seconds_since_fix": None,
    "accel_mean": None,
    "accel_variance": None,
    "accel_stats_x": {},
    "accel_stats_y": {},
    "accel_stats_z": {},
}
data_lock = threading.Lock() # To safely update and read latest_data

# --- Flask App Definition ---
app = Flask(__name__)
CORS(app) # Enable CORS for all routes

@app.route('/data', methods=['GET'])
def get_data():
    with data_lock:
        # latest_data no longer contains 'raw_lines'
        return jsonify(latest_data)

@app.route('/', methods=['GET'])
def get_status():
    global S3_BUS_TYPE_PREFIX
    return jsonify({
        "status": "Serial to API bridge running. Access data at /data. Uploading to S3.",
        "bus_type_configured": S3_BUS_TYPE_PREFIX # Show the configured bus type
    })

# --- Function to Send Data to AWS S3 ---
def send_to_external_storage(data_to_send):
    """
    Uploads the provided data dictionary as a JSON file to AWS S3.
    The filename will be timestamped.
    """
    global S3_BUS_TYPE_PREFIX # Access the global variable
    try:
        # data_to_send should already be clean (no raw_lines), but good practice to ensure.
        if 'raw_lines' in data_to_send:
            # This should not happen if parsed_block is correctly constructed
            # and latest_data structure doesn't include it.
            clean_data = data_to_send.copy()
            del clean_data['raw_lines']
        else:
            clean_data = data_to_send

        json_data_for_s3 = json.dumps(clean_data)

        # Create a unique filename using the timestamp from the data or current UTC time
        data_timestamp_str = clean_data.get('timestamp') # e.g., "2025-03-30 17:23:59"
        if data_timestamp_str:
            # Make filename more S3-friendly (no spaces, colons)
            # Assuming timestamp is like "YYYY-MM-DD HH:MM:SS"
            s3_timestamp = data_timestamp_str.replace(' ', 'T').replace(':', '-')
        else:
            s3_timestamp = datetime.datetime.utcnow().strftime('%Y-%m-%dT%H-%M-%SZ')

        # TODO: change this to the bus type when you begin data collection
        filename = f"sensor_data/{S3_BUS_TYPE_PREFIX}/data_{s3_timestamp}.json"

        s3_client.put_object(Bucket=AWS_S3_BUCKET, Key=filename, Body=json_data_for_s3, ContentType='application/json')
        print(f"Successfully uploaded {filename} to S3 bucket {AWS_S3_BUCKET}")

    except (NoCredentialsError, PartialCredentialsError):
        print("AWS S3 Error: Credentials not found or incomplete. Please configure AWS credentials.")
    except Exception as e:
        print(f"AWS S3 Error: Failed to upload data to S3: {e}")


# --- Serial Data Reader and Parser Thread ---
def read_serial_data():
    global latest_data
    
    while True:
        try:
            print(f"Attempting to connect to serial port {SERIAL_PORT} at {BAUD_RATE} bps...")
            with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
                print(f"Connected to {SERIAL_PORT}. Reading data...")
                current_block_lines = []
                while True:
                    try:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                    except serial.SerialException as e:
                        print(f"Serial error: {e}. Reconnecting...")
                        break # Break inner loop to reconnect
                    except UnicodeDecodeError:
                        continue

                    if line:
                        current_block_lines.append(line)

                        if "-------------------------------------------------------------------------------" in line and len(current_block_lines) > 1:
                            parsed_block = parse_sensor_block(current_block_lines)
                            
                            if parsed_block:
                                # parsed_block does not contain 'raw_lines' as per parse_sensor_block definition
                                
                                # Update the global latest_data for the Flask API
                                with data_lock:
                                    latest_data.update(parsed_block)
                                    # print(f"Updated local API data at: {latest_data.get('timestamp')}") # For debugging

                                # Send a copy of the parsed_block to S3
                                # (parsed_block itself is clean and doesn't include raw_lines)
                                send_to_external_storage(parsed_block.copy()) # Send a copy to be safe
                            
                            current_block_lines = [] # Reset for the next block
                        
                        if len(current_block_lines) > 30: # Safety net
                            current_block_lines = []

        except serial.SerialException:
            print(f"Failed to connect to {SERIAL_PORT}. Retrying in 5 seconds...")
            time.sleep(5)
        except Exception as e:
            print(f"An unexpected error occurred in serial thread: {e}")
            time.sleep(5)

def parse_percentiles(line):
    match = re.search(r"p1=([\d.-]+), p10=([\d.-]+), p90=([\d.-]+), p99=([\d.-]+)", line)
    if match:
        return {
            "p1": float(match.group(1)),
            "p10": float(match.group(2)),
            "p90": float(match.group(3)),
            "p99": float(match.group(4)),
        }
    return {}

def parse_sensor_block(block_lines):
    # This function now returns a dictionary that ONLY contains the parsed sensor values.
    # It does NOT include 'raw_lines'.
    data = {}
    try:
        for line in block_lines:
            if "Date/Time:" in line:
                match = re.search(r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})", line)
                if match:
                    data["timestamp"] = match.group(1)
            elif "GPS: Lat:" in line:
                match = re.search(r"Lat: ([\d.-]+), Lon: ([\d.-]+), Alt: ([\d.-]+)", line)
                if match:
                    data["gps_fix_valid"] = True
                    data["latitude"] = float(match.group(1))
                    data["longitude"] = float(match.group(2))
                    data["altitude"] = float(match.group(3))
            elif "Speed: %.2f m/s, Bearing: %.1f°" in line:
                match = re.search(r"Speed: ([\d.-]+) m/s, Bearing: ([\d.-]+)°", line)
                if match:
                    data["speed"] = float(match.group(1))
                    data["bearing"] = float(match.group(2))
            elif "GPS: Searching" in line:
                data["gps_fix_valid"] = False
                match = re.search(r"No fix for (\d+) seconds", line)
                if match:
                    data["seconds_since_fix"] = int(match.group(1))
            elif "Mean (Magnitude):" in line:
                match = re.search(r"Mean \(Magnitude\): ([\d.-]+)", line)
                if match:
                    data["accel_mean"] = float(match.group(1))
            elif "Variance (Magnitude):" in line:
                match = re.search(r"Variance \(Magnitude\): ([\d.-]+)", line)
                if match:
                    data["accel_variance"] = float(match.group(1))
            elif "X-Axis:" in line:
                data["accel_stats_x"] = parse_percentiles(line)
            elif "Y-Axis:" in line:
                data["accel_stats_y"] = parse_percentiles(line)
            elif "Z-Axis:" in line:
                data["accel_stats_z"] = parse_percentiles(line)
        
        if not data.get("gps_fix_valid", False):
            data.setdefault("latitude", 0.0)
            data.setdefault("longitude", 0.0)
            data.setdefault("altitude", 0.0)
            data.setdefault("speed", 0.0)
            data.setdefault("bearing", 0.0)
            data.setdefault("seconds_since_fix", data.get("seconds_since_fix", 0))


    except Exception as e:
        print(f"Error parsing block: {e}\nBlock was: {block_lines}")
        return None
    return data

# --- Main Execution ---
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Serial to API bridge with S3 upload. Takes bus_type as an argument.")
    parser.add_argument('bus_type', type=str, help='Identifier for the bus type/trip (e.g., "gold", "blue"). This will be used in the S3 path.')
    args = parser.parse_args()

    # Set the global S3_BUS_TYPE_PREFIX from the command line argument
    S3_BUS_TYPE_PREFIX = args.bus_type

    print("Starting sensor data to API bridge with S3 upload.")
    print(f"Configured for bus type: {S3_BUS_TYPE_PREFIX}")
    print(f"Ensure AWS credentials are configured for S3 bucket '{AWS_S3_BUCKET}' in region '{AWS_S3_REGION}'.")
    
    serial_thread = threading.Thread(target=read_serial_data, daemon=True)
    serial_thread.start()

    print(f"Starting Flask server on http://0.0.0.0:5000")
    app.run(host='0.0.0.0', port=5000, debug=False) # debug=False for less verbose Flask logs usually

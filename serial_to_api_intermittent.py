import serial
import threading
import time
import re
from flask import Flask, jsonify
from flask_cors import CORS
import boto3
import json
import datetime
from botocore.exceptions import NoCredentialsError, PartialCredentialsError, BotoCoreError, ClientError
import os
import atexit # To save queue on exit

# --- Configuration ---
SERIAL_PORT = 'COM3'
BAUD_RATE = 115200

AWS_S3_BUCKET = 'gt-stinger-bucket' # Your S3 bucket
AWS_S3_REGION = 'us-east-2'       # Your S3 region

# --- Batching/Offline Queue Configuration ---
OFFLINE_QUEUE_FILE = 's3_offline_queue.json'
MAX_QUEUE_SIZE = 10000  # Max items to hold in memory before aggressively trying to save
RETRY_UPLOAD_INTERVAL_SECONDS = 60 # How often to try uploading queued items
LOCAL_SAVE_INTERVAL_SECONDS = 300 # How often to save the in-memory queue to disk

# --- Globals ---
s3_client = boto3.client('s3', region_name=AWS_S3_REGION)
latest_data = {
    "timestamp": None, "gps_fix_valid": False, "latitude": None, "longitude": None,
    "altitude": None, "speed": None, "bearing": None, "seconds_since_fix": None,
    "accel_mean": None, "accel_variance": None, "accel_stats_x": {},
    "accel_stats_y": {}, "accel_stats_z": {},
}
data_lock = threading.Lock()
s3_upload_queue = []
s3_queue_lock = threading.Lock()
last_local_save_time = time.time()

# --- Flask App ---
app = Flask(__name__)
CORS(app)

@app.route('/data', methods=['GET'])
def get_data_route(): # Renamed to avoid conflict
    with data_lock:
        return jsonify(latest_data)

@app.route('/', methods=['GET'])
def get_status():
    with s3_queue_lock:
        q_size = len(s3_upload_queue)
    return jsonify({"status": "Serial to API bridge running.",
                    "s3_upload_queue_size": q_size,
                    "uploading_to_s3_bucket": AWS_S3_BUCKET})

# --- S3 and Queue Management ---
def save_queue_to_disk():
    global last_local_save_time
    with s3_queue_lock:
        if not s3_upload_queue: # Don't write empty file if queue is empty and file exists
            if os.path.exists(OFFLINE_QUEUE_FILE):
                print(f"Queue is empty, removing old {OFFLINE_QUEUE_FILE}")
                try:
                    os.remove(OFFLINE_QUEUE_FILE)
                except OSError as e:
                    print(f"Error removing empty queue file: {e}")
            return

        try:
            with open(OFFLINE_QUEUE_FILE, 'w') as f:
                json.dump(s3_upload_queue, f)
            print(f"Saved {len(s3_upload_queue)} items to {OFFLINE_QUEUE_FILE}")
            last_local_save_time = time.time()
        except IOError as e:
            print(f"Error saving queue to disk: {e}")

def load_queue_from_disk():
    global s3_upload_queue
    if os.path.exists(OFFLINE_QUEUE_FILE):
        try:
            with open(OFFLINE_QUEUE_FILE, 'r') as f:
                s3_upload_queue = json.load(f)
            print(f"Loaded {len(s3_upload_queue)} items from {OFFLINE_QUEUE_FILE}")
        except (IOError, json.JSONDecodeError) as e:
            print(f"Error loading queue from disk: {e}. Starting with an empty queue.")
            s3_upload_queue = []

def add_to_s3_queue(data_item):
    with s3_queue_lock:
        if len(s3_upload_queue) < MAX_QUEUE_SIZE:
            s3_upload_queue.append(data_item)
        else:
            print("S3 upload queue is full. Discarding oldest item or new item based on strategy (currently new).")
            # Simple strategy: discard new. Could also discard oldest: s3_upload_queue.pop(0); s3_upload_queue.append(data_item)

        # Save to disk if it's been a while
        if time.time() - last_local_save_time > LOCAL_SAVE_INTERVAL_SECONDS:
            save_queue_to_disk()


def attempt_s3_upload(data_to_send):
    """Attempts to upload a single data item to S3. Returns True on success, False on failure."""
    try:
        clean_data = data_to_send.copy() # data_to_send is already a dict
        json_data_for_s3 = json.dumps(clean_data)
        data_timestamp_str = clean_data.get('timestamp')
        s3_timestamp = data_timestamp_str.replace(' ', 'T').replace(':', '-') if data_timestamp_str else datetime.datetime.utcnow().strftime('%Y-%m-%dT%H-%M-%SZ')
        
        # Customize your S3 key structure here
        # Example: sensor_data/bus_route_123/YYYY/MM/DD/data_timestamp.json
        now = datetime.datetime.strptime(data_timestamp_str, "%Y-%m-%d %H:%M:%S") if data_timestamp_str else datetime.datetime.utcnow()
        s3_key = f"sensor_data/bus_trip_alpha/{now.strftime('%Y/%m/%d')}/data_{s3_timestamp}.json"
        
        s3_client.put_object(Bucket=AWS_S3_BUCKET, Key=s3_key, Body=json_data_for_s3, ContentType='application/json')
        print(f"Successfully uploaded {s3_key} to S3.")
        return True
    except (NoCredentialsError, PartialCredentialsError):
        print("AWS S3 Error: Credentials not found or incomplete.")
        return False # Credentials error is not transient
    except (BotoCoreError, ClientError) as e: # Catch common boto3 network/service errors
        print(f"AWS S3 Error during upload: {e}")
        return False # Transient error
    except Exception as e:
        print(f"Unexpected error during S3 upload: {e}")
        return False

def process_s3_queue():
    """Periodically tries to upload items from the queue."""
    while True:
        items_to_retry = []
        with s3_queue_lock:
            if s3_upload_queue:
                # Take a few items to retry, e.g., up to 5, to avoid holding lock too long
                count = min(len(s3_upload_queue), 5)
                items_to_retry = [s3_upload_queue.pop(0) for _ in range(count)]

        if not items_to_retry:
            time.sleep(RETRY_UPLOAD_INTERVAL_SECONDS)
            continue

        print(f"Attempting to upload {len(items_to_retry)} items from S3 queue.")
        successful_uploads = 0
        failed_items_this_round = []

        for item in items_to_retry:
            if attempt_s3_upload(item):
                successful_uploads += 1
            else:
                failed_items_this_round.append(item)
        
        if failed_items_this_round:
            with s3_queue_lock: # Add back to the front of the queue if failed
                s3_upload_queue[:0] = failed_items_this_round
            print(f"Re-queued {len(failed_items_this_round)} items after failed S3 upload attempt.")

        if successful_uploads > 0 or (items_to_retry and not failed_items_this_round): # If anything changed in queue or uploads happened
             # Save disk more frequently if queue is actively changing
            if len(s3_upload_queue) < 50 or successful_uploads > 0 : # Save if queue is small or we just uploaded
                 save_queue_to_disk()


        time.sleep(RETRY_UPLOAD_INTERVAL_SECONDS)


# Renamed to avoid conflict with Python's built-in `send`
def send_data_to_storage_handler(data_to_send):
    if not attempt_s3_upload(data_to_send):
        print(f"Failed initial S3 upload for timestamp {data_to_send.get('timestamp')}. Adding to offline queue.")
        add_to_s3_queue(data_to_send)

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
                        break 
                    except UnicodeDecodeError:
                        continue

                    if line:
                        current_block_lines.append(line)
                        if "-------------------------------------------------------------------------------" in line and len(current_block_lines) > 1:
                            parsed_block = parse_sensor_block(current_block_lines)
                            if parsed_block:
                                with data_lock:
                                    latest_data.update(parsed_block)
                                send_data_to_storage_handler(parsed_block.copy())
                            current_block_lines = []
                        
                        if len(current_block_lines) > 30:
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
            "p1": float(match.group(1)), "p10": float(match.group(2)),
            "p90": float(match.group(3)), "p99": float(match.group(4)),
        }
    return {}

def parse_sensor_block(block_lines):
    data = {}
    try:
        for line in block_lines:
            if "Date/Time:" in line:
                match = re.search(r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})", line)
                if match: data["timestamp"] = match.group(1)
            elif "GPS: Lat:" in line:
                match = re.search(r"Lat: ([\d.-]+), Lon: ([\d.-]+), Alt: ([\d.-]+)", line)
                if match:
                    data.update({"gps_fix_valid": True, "latitude": float(match.group(1)),
                                 "longitude": float(match.group(2)), "altitude": float(match.group(3))})
            elif "Speed: %.2f m/s, Bearing: %.1f°" in line:
                match = re.search(r"Speed: ([\d.-]+) m/s, Bearing: ([\d.-]+)°", line)
                if match: data.update({"speed": float(match.group(1)), "bearing": float(match.group(2))})
            elif "GPS: Searching" in line:
                data["gps_fix_valid"] = False
                match = re.search(r"No fix for (\d+) seconds", line)
                if match: data["seconds_since_fix"] = int(match.group(1))
            elif "Mean (Magnitude):" in line:
                match = re.search(r"Mean \(Magnitude\): ([\d.-]+)", line)
                if match: data["accel_mean"] = float(match.group(1))
            elif "Variance (Magnitude):" in line:
                match = re.search(r"Variance \(Magnitude\): ([\d.-]+)", line)
                if match: data["accel_variance"] = float(match.group(1))
            elif "X-Axis:" in line: data["accel_stats_x"] = parse_percentiles(line)
            elif "Y-Axis:" in line: data["accel_stats_y"] = parse_percentiles(line)
            elif "Z-Axis:" in line: data["accel_stats_z"] = parse_percentiles(line)
        
        if not data.get("gps_fix_valid", False):
            data.setdefault("latitude", 0.0); data.setdefault("longitude", 0.0)
            data.setdefault("altitude", 0.0); data.setdefault("speed", 0.0)
            data.setdefault("bearing", 0.0)
            data.setdefault("seconds_since_fix", data.get("seconds_since_fix", 0))
    except Exception as e:
        print(f"Error parsing block: {e}\nBlock was: {block_lines}")
        return None
    return data

# --- Main Execution ---
if __name__ == '__main__':
    print("Starting sensor data to API bridge with S3 upload and offline queue.")
    load_queue_from_disk()
    atexit.register(save_queue_to_disk) # Ensure queue is saved on normal exit

    print(f"Ensure AWS credentials are configured for S3 bucket '{AWS_S3_BUCKET}' in region '{AWS_S3_REGION}'.")
    
    s3_retry_thread = threading.Thread(target=process_s3_queue, daemon=True)
    s3_retry_thread.start()

    serial_thread = threading.Thread(target=read_serial_data, daemon=True)
    serial_thread.start()

    print(f"Starting Flask server on http://0.0.0.0:5000")
    app.run(host='0.0.0.0', port=5000, debug=False)
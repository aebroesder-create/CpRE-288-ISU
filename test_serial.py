# test_serial.py
import serial
import time


COM_PORT = "COM14"

try:
    print(f"Opening {COM_PORT}...")
    s = serial.Serial(COM_PORT, 115200, timeout=2)
    print("Connected!")
    
    # Send 'g' command
    s.write(b'g')
    print("Sent 'g'")
    
    # Read for 5 seconds
    start = time.time()
    while time.time() - start < 5:
        if s.in_waiting:
            line = s.readline().decode('utf-8', errors='ignore').strip()
            print(f"Got: {line}")
    
    s.close()
    print("Done")
    
except Exception as e:
    print(f"Error: {e}")
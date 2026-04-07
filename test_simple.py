import socket
import time

CYBOT_IP = '192.168.1.1'
PORT = 288

print(f"Connecting to {CYBOT_IP}:{PORT}...")
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)

try:
    sock.connect((CYBOT_IP, PORT))
    print("Connected!")
    
    sock.send(b'g')
    print("Sent 'g'")
    
    print("Waiting for data (10 seconds)...")
    start = time.time()
    while time.time() - start < 10:
        try:
            data = sock.recv(1024)
            if data:
                print(f"RECEIVED: {data.decode('utf-8', errors='replace')}")
            else:
                break
        except socket.timeout:
            continue
        except Exception as e:
            print(f"Error reading: {e}")
            break
            
except Exception as e:
    print(f"Error: {e}")
finally:
    sock.close()
    print("Done")
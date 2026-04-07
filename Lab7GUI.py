import socket
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
from matplotlib.widgets import Button

CYBOT_IP = '192.168.1.1'
PORT = 288

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setblocking(False)
try:
    print(f"Connecting to CyBot at {CYBOT_IP}...")
    sock.connect((CYBOT_IP, PORT))
except BlockingIOError:
    pass

print("Socket ready (non-blocking)")

# State variables
scanning = False          # Are we currently collecting a scan?
ignore_data = False       # Ignore incoming data (after Stop pressed)
buffer = ""
angles = []
distances = []

fig, ax = plt.subplots(subplot_kw={'projection': 'polar'})
ax.set_thetamax(180)
ax.set_theta_zero_location("N")
ax.set_theta_direction(-1)
ax.set_rmax(500)
ax.set_rticks([100,200,300,400,500])
ax.set_rlabel_position(-22.5)

text_display = ax.text(
    0.05, 0.95, 'Ready - Press Start',
    transform=ax.transAxes,
    verticalalignment='top',
    fontsize=10,
    color='blue'
)

def clear_plot():
    """Completely clear the plot and reset data"""
    global angles, distances
    angles = []
    distances = []
    ax.clear()
    ax.set_thetamax(180)
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)
    ax.set_rmax(500)
    ax.set_rticks([100,200,300,400,500])
    ax.set_rlabel_position(-22.5)
    text_display.set_text("Ready - Press Start")
    plt.draw()
    print("Plot cleared - ready for new scan")

def start(event):
    global scanning, ignore_data, buffer, angles, distances
    
    # Reset everything for new scan
    ignore_data = False
    scanning = True
    buffer = ""
    angles = []
    distances = []
    
    # Clear the plot
    clear_plot()
    text_display.set_text("Scanning...")
    plt.draw()
    
    try:
        sock.send(b'g')
        print("\n=== Sent 'g' - Starting NEW scan ===")
    except Exception as e:
        print(f"Failed to send: {e}")
        scanning = False

def stop(event):
    global scanning, ignore_data
    
    if scanning:
        print("\n=== STOP pressed - Ignoring remaining data from current scan ===")
        scanning = False
        ignore_data = True
        text_display.set_text(f"Stopped - Data ignored. Press Start for new scan")
        print(f"Current points collected before stop: {len(angles)}")
        # Don't clear the plot - keep showing what was collected
        plt.draw()
    else:
        # Just clear the plot if no scan is happening
        clear_plot()

# Start/Stop Buttons
ax_start = plt.axes([0.7, 0.05, 0.1, 0.05])
ax_stop  = plt.axes([0.81, 0.05, 0.1, 0.05])
btn_start = Button(ax_start, 'Start')
btn_stop  = Button(ax_stop, 'Stop')
btn_start.on_clicked(start)
btn_stop.on_clicked(stop)

def update_plot():
    """Redraw the plot with current points"""
    if not angles:
        return
    
    ax.clear()
    ax.set_thetamax(180)
    ax.set_theta_zero_location("N")
    ax.set_theta_direction(-1)
    ax.set_rmax(500)
    ax.set_rticks([100,200,300,400,500])
    ax.set_rlabel_position(-22.5)
    
    angle_rad = np.deg2rad(angles)
    ax.plot(angle_rad, distances, 'r-', linewidth=2, marker='o', markersize=4)
    
    if angles:
        text_display.set_text(f"Points: {len(angles)}\nAngle: {int(angles[-1])}°\nDist: {int(distances[-1])}cm")
    
    plt.draw()

def update(frame):
    global buffer, angles, distances, scanning, ignore_data
    
    if ignore_data:
        # If we're ignoring data, just read and discard
        try:
            sock.recv(1024)
        except:
            pass
        return

    if not scanning:
        return

    try:
        data = sock.recv(1024).decode('utf-8', errors='replace')
        if not data:
            return
        
        buffer += data
        
        while "\n" in buffer:
            line, buffer = buffer.split("\n", 1)
            line = line.strip()
            
            if not line:
                continue
            
            # Check for scan start
            if "START_SCAN" in line:
                print("Scan started - collecting data...")
                angles = []
                distances = []
            
            # Parse CSV data: angle,distance,ir_raw
            elif "," in line and "START" not in line and "COMPLETE" not in line and "Angle" not in line:
                try:
                    parts = line.split(',')
                    if len(parts) >= 2:
                        angle_deg = float(parts[0])
                        dist = float(parts[1])
                        angles.append(angle_deg)
                        distances.append(dist)
                        print(f"Point {len(angles)}: {angle_deg}°, {dist:.1f}cm")
                        update_plot()
                except ValueError:
                    pass
            
            # Check for scan complete
            elif "SCAN_COMPLETE" in line:
                print(f"\n=== SCAN COMPLETE! {len(angles)} points collected ===")
                text_display.set_text(f"Complete! {len(angles)} points")
                scanning = False
                ignore_data = False
            
            # Print other messages (excluding header)
            elif line and not line.startswith(">>>") and not line.startswith("Press") and line != "Angle,Distance,IR_Raw":
                print(f"Bot: {line}")
                
    except BlockingIOError:
        pass
    except ConnectionResetError:
        print("Connection lost")
        scanning = False
        ignore_data = False
        return

ani = animation.FuncAnimation(fig, update, interval=50, cache_frame_data=False)
plt.show()
sock.close()
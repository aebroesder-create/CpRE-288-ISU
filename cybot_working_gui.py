"""
CyBot Working GUI - Real-time Scan and Plot
===========================================
This GUI sends 's' to the CyBot and receives scan data in real-time,
then automatically plots it.

Works with the updated CybotGUIScan.c code above.
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import math
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk

class CyBotWorkingGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("CyBot Real-Time Scan GUI")
        self.root.geometry("1100x750")
        self.root.configure(bg="#2b2b2b")
        
        # Serial connection variables
        self.ser = None
        self.connected = False
        self.running = True
        
        # Scan data storage
        self.angles = []
        self.distances = []
        self.is_scanning = False
        
        # Build the GUI
        self.setup_gui()
        
        # Auto-detect COM port
        self.detect_com_port()
        
        # Start the serial reader thread
        self.reader_thread = threading.Thread(target=self.serial_reader, daemon=True)
        self.reader_thread.start()
    
    def setup_gui(self):
        """Setup the GUI layout"""
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # === Top Frame - Connection Controls ===
        top_frame = ttk.LabelFrame(main_frame, text="Connection", padding="10")
        top_frame.pack(fill=tk.X, pady=(0, 10))
        
        # COM Port
        ttk.Label(top_frame, text="COM Port:").grid(row=0, column=0, padx=5)
        self.com_port_var = tk.StringVar()
        self.com_port_combo = ttk.Combobox(top_frame, textvariable=self.com_port_var, 
                                           width=10, state="readonly")
        self.com_port_combo.grid(row=0, column=1, padx=5)
        
        # Baud Rate
        ttk.Label(top_frame, text="Baud Rate:").grid(row=0, column=2, padx=5)
        self.baud_var = tk.StringVar(value="115200")
        baud_combo = ttk.Combobox(top_frame, textvariable=self.baud_var,
                                  values=["9600", "19200", "38400", "115200"],
                                  width=10, state="readonly")
        baud_combo.grid(row=0, column=3, padx=5)
        
        # Connect/Disconnect buttons
        self.connect_btn = ttk.Button(top_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=4, padx=10)
        
        self.disconnect_btn = ttk.Button(top_frame, text="Disconnect", 
                                         command=self.disconnect, state=tk.DISABLED)
        self.disconnect_btn.grid(row=0, column=5, padx=5)
        
        # Status
        self.status_label = ttk.Label(top_frame, text="● Disconnected", foreground="red")
        self.status_label.grid(row=0, column=6, padx=20)
        
        # === Middle Frame - Plot and Data ===
        middle_frame = ttk.Frame(main_frame)
        middle_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # Left side - Polar Plot
        plot_frame = ttk.LabelFrame(middle_frame, text="Polar Plot", padding="5")
        plot_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Create matplotlib figure for polar plot
        self.fig = Figure(figsize=(6, 5), facecolor="#f0f0f0")
        self.ax = self.fig.add_subplot(111, projection='polar')
        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # Add toolbar
        toolbar = NavigationToolbar2Tk(self.canvas, plot_frame)
        toolbar.update()
        
        # Right side - Control Panel
        control_frame = ttk.LabelFrame(middle_frame, text="Controls", padding="10")
        control_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(10, 0))
        
        # Scan button
        self.scan_btn = ttk.Button(control_frame, text="START SCAN", 
                                   command=self.start_scan, width=20,
                                   state=tk.DISABLED)
        self.scan_btn.pack(pady=10)
        
        # Clear button
        self.clear_btn = ttk.Button(control_frame, text="Clear Plot", 
                                    command=self.clear_plot, width=20)
        self.clear_btn.pack(pady=5)
        
        # Save button
        self.save_btn = ttk.Button(control_frame, text="Save Data", 
                                   command=self.save_data, width=20,
                                   state=tk.DISABLED)
        self.save_btn.pack(pady=5)
        
        # Data display
        ttk.Label(control_frame, text="Scan Data:", font=("Arial", 10, "bold")).pack(pady=(10, 5))
        self.data_text = scrolledtext.ScrolledText(control_frame, height=15, width=30)
        self.data_text.pack(fill=tk.BOTH, expand=True)
        
        # === Bottom Frame - Serial Terminal ===
        bottom_frame = ttk.LabelFrame(main_frame, text="Serial Terminal", padding="5")
        bottom_frame.pack(fill=tk.X)
        
        self.terminal = scrolledtext.ScrolledText(bottom_frame, height=6, bg="black", fg="lime")
        self.terminal.pack(fill=tk.X)
        
        # Command entry
        cmd_frame = ttk.Frame(bottom_frame)
        cmd_frame.pack(fill=tk.X, pady=(5, 0))
        
        self.cmd_entry = ttk.Entry(cmd_frame)
        self.cmd_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.cmd_entry.bind('<Return>', lambda e: self.send_command())
        
        self.send_btn = ttk.Button(cmd_frame, text="Send", command=self.send_command,
                                   state=tk.DISABLED)
        self.send_btn.pack(side=tk.RIGHT, padx=(5, 0))
        
        # Initialize plot
        self.init_plot()
    
    def init_plot(self):
        """Initialize the polar plot"""
        self.ax.clear()
        self.ax.set_facecolor("#e0e0e0")
        self.ax.set_thetamin(0)
        self.ax.set_thetamax(180)
        self.ax.set_theta_zero_location("W")
        self.ax.set_theta_direction(-1)
        self.ax.set_rmax(2.5)
        self.ax.set_rticks([0.5, 1.0, 1.5, 2.0, 2.5])
        self.ax.set_xticks(np.radians([0, 45, 90, 135, 180]))
        self.ax.set_xticklabels(['0°', '45°', '90°', '135°', '180°'])
        self.ax.grid(True)
        self.ax.set_title("Distance (meters)", fontsize=10)
        self.canvas.draw()
    
    def detect_com_port(self):
        """Auto-detect available COM ports"""
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.com_port_combo['values'] = ports
        if ports:
            self.com_port_combo.set(ports[0])
    
    def toggle_connection(self):
        """Connect to the CyBot"""
        if not self.connected:
            try:
                port = self.com_port_var.get()
                baud = int(self.baud_var.get())
                self.ser = serial.Serial(port, baud, timeout=0.1)
                self.connected = True
                
                self.connect_btn.config(state=tk.DISABLED)
                self.disconnect_btn.config(state=tk.NORMAL)
                self.scan_btn.config(state=tk.NORMAL)
                self.send_btn.config(state=tk.NORMAL)
                self.status_label.config(text="● Connected", foreground="green")
                
                self.log_message(f"Connected to {port} at {baud} baud")
                
            except Exception as e:
                messagebox.showerror("Error", f"Failed to connect: {str(e)}")
                self.log_message(f"ERROR: {str(e)}")
        else:
            self.disconnect()
    
    def disconnect(self):
        """Disconnect from CyBot"""
        self.connected = False
        if self.ser:
            self.ser.close()
            self.ser = None
        
        self.connect_btn.config(state=tk.NORMAL)
        self.disconnect_btn.config(state=tk.DISABLED)
        self.scan_btn.config(state=tk.DISABLED)
        self.send_btn.config(state=tk.DISABLED)
        self.status_label.config(text="● Disconnected", foreground="red")
        
        self.log_message("Disconnected")
    
    def start_scan(self):
        """Send scan command to CyBot"""
        if not self.connected:
            messagebox.showwarning("Not Connected", "Please connect to CyBot first")
            return
        
        # Clear previous data
        self.angles = []
        self.distances = []
        self.data_text.delete(1.0, tk.END)
        self.is_scanning = True
        
        # Send 's' command
        self.ser.write(b's')
        self.log_message("Scan command sent - waiting for data...")
    
    def update_plot(self):
        """Update the polar plot with current data"""
        if not self.angles:
            return
        
        self.ax.clear()
        
        # Convert to radians
        rads = np.radians(self.angles)
        
        # Plot
        self.ax.plot(rads, self.distances, 'b-', linewidth=2, marker='o', markersize=4)
        self.ax.fill(rads, self.distances, alpha=0.3)
        
        # Restore settings
        self.ax.set_facecolor("#e0e0e0")
        self.ax.set_thetamin(0)
        self.ax.set_thetamax(180)
        self.ax.set_theta_zero_location("W")
        self.ax.set_theta_direction(-1)
        self.ax.set_rmax(2.5)
        self.ax.set_rticks([0.5, 1.0, 1.5, 2.0, 2.5])
        self.ax.set_xticks(np.radians([0, 45, 90, 135, 180]))
        self.ax.set_xticklabels(['0°', '45°', '90°', '135°', '180°'])
        self.ax.grid(True)
        self.ax.set_title("Distance (meters)", fontsize=10)
        
        self.canvas.draw()
        
        # Enable save button
        self.save_btn.config(state=tk.NORMAL)
    
    def clear_plot(self):
        """Clear the plot and data"""
        self.angles = []
        self.distances = []
        self.data_text.delete(1.0, tk.END)
        self.init_plot()
        self.save_btn.config(state=tk.DISABLED)
        self.log_message("Plot cleared")
    
    def save_data(self):
        """Save scan data to file"""
        if not self.angles:
            messagebox.showwarning("No Data", "No scan data to save")
            return
        
        from tkinter import filedialog
        filename = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write("Angle(Degrees)\tDistance(m)\n")
                    for a, d in zip(self.angles, self.distances):
                        f.write(f"{a}\t\t{d:.2f}\n")
                    f.write("END\n")
                self.log_message(f"Data saved to {filename}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save: {str(e)}")
    
    def send_command(self):
        """Send manual command to CyBot"""
        if not self.connected:
            return
        
        cmd = self.cmd_entry.get()
        if cmd:
            self.ser.write(cmd.encode())
            self.log_message(f"Sent: {cmd}")
            self.cmd_entry.delete(0, tk.END)
    
    def log_message(self, message):
        """Add message to terminal"""
        self.terminal.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {message}\n")
        self.terminal.see(tk.END)
    
    def serial_reader(self):
        """Thread function to read from serial port"""
        buffer = ""
        
        while self.running:
            if self.connected and self.ser and self.ser.is_open:
                try:
                    if self.ser.in_waiting:
                        data = self.ser.read(self.ser.in_waiting)
                        text = data.decode('utf-8', errors='ignore')
                        
                        # Display in terminal
                        self.root.after(0, self.terminal.insert, tk.END, text)
                        self.root.after(0, self.terminal.see, tk.END)
                        
                        # Parse data if scanning
                        if self.is_scanning:
                            buffer += text
                            lines = buffer.split('\n')
                            buffer = lines[-1]
                            
                            for line in lines[:-1]:
                                line = line.strip()
                                
                                # Check for start of data
                                if "Angle(Degrees)" in line:
                                    self.angles = []
                                    self.distances = []
                                    self.log_message("Receiving scan data...")
                                
                                # Parse data line
                                elif line and not line.startswith("SCAN") and not line.startswith("END"):
                                    parts = line.split()
                                    if len(parts) >= 2:
                                        try:
                                            angle = float(parts[0])
                                            dist = float(parts[1])
                                            self.angles.append(angle)
                                            self.distances.append(dist)
                                            
                                            # Update data display
                                            self.root.after(0, self.data_text.insert, tk.END, 
                                                          f"{angle:3.0f}°  {dist:5.2f}m\n")
                                            self.root.after(0, self.data_text.see, tk.END)
                                        except:
                                            pass
                                
                                # Scan complete
                                elif "SCAN_END" in line or "END" in line:
                                    self.is_scanning = False
                                    self.root.after(0, self.update_plot)
                                    self.root.after(0, self.log_message, 
                                                  f"Scan complete! {len(self.angles)} points collected")
                
                except Exception as e:
                    if self.running:
                        self.root.after(0, self.log_message, f"Serial error: {str(e)}")
            
            time.sleep(0.05)
    
    def on_closing(self):
        """Clean up on exit"""
        self.running = False
        if self.ser:
            self.ser.close()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = CyBotWorkingGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()

if __name__ == "__main__":
    main()
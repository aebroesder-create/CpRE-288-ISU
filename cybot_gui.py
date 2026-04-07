"""
CyBot Ping Sweep GUI with Data Saving and Polar Plot
====================================================
Works just like Putty - type 's' in the input box to trigger a scan.
The polar plot updates automatically when scan data is received.
Data is saved to a file that can be used with the polar plot example.

SETUP:
  pip install pyserial matplotlib

USAGE:
  1. Close Putty completely
  2. Flash CybotGUIScan.c to CyBot and confirm LCD says "Press s to scan"
  3. Run: python cybot_gui.py
  4. Click Connect
  5. Type 's' in the input box at the bottom and press Enter (or click Send)
  6. Watch the plot update
  7. Use "Save Data" to save the scan to a file
  8. Use "Load & Plot" to load and plot saved data
"""

import tkinter as tk
from tkinter import scrolledtext, filedialog, messagebox, ttk
import serial
import serial.tools.list_ports
import threading
import time
import math
import os
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk

# ── CHANGE THIS IF NEEDED ──────────────────────────
COM_PORT  = "COM19"
BAUD_RATE = 115200
# ───────────────────────────────────────────────────


class CyBotGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("CyBot Ping Sweep with Data Logging")
        self.root.geometry("1100x750")
        self.root.configure(bg="#2b2b2b")

        self.ser = None
        self.running = False
        self.current_angles = []
        self.current_distances = []
        self.scan_data = []  # Store (angle, distance) pairs for saving
        
        # Setup paths for data storage
        self.absolute_path = os.path.dirname(__file__)
        self.relative_path = "./"
        self.full_path = os.path.join(self.absolute_path, self.relative_path)
        self.filename = 'cybot-sensor-scan.txt'

        self._build_ui()
        self._auto_detect_port()

    # ── BUILD UI ───────────────────────────────────
    def _build_ui(self):
        # ── Top bar: title + connection controls
        top = tk.Frame(self.root, bg="#2b2b2b")
        top.pack(fill="x", padx=10, pady=8)

        tk.Label(top, text="CyBot Ping Sweep with Data Logging", 
                 font=("Helvetica", 15, "bold"),
                 bg="#2b2b2b", fg="white").pack(side="left")

        tk.Label(top, text="Port:", bg="#2b2b2b", fg="#aaa",
                 font=("Helvetica", 10)).pack(side="left", padx=(30, 4))

        self.port_var = tk.StringVar(value=COM_PORT)
        self.port_entry = tk.Entry(top, textvariable=self.port_var, width=8,
                                   bg="#3c3c3c", fg="white",
                                   insertbackground="white",
                                   relief="flat", font=("Courier", 10))
        self.port_entry.pack(side="left", padx=(0, 6))

        self.btn_connect = tk.Button(top, text="Connect",
                                     command=self._connect,
                                     bg="#4caf50", fg="white",
                                     font=("Helvetica", 9, "bold"),
                                     relief="flat", padx=10, cursor="hand2")
        self.btn_connect.pack(side="left", padx=2)

        self.btn_disconnect = tk.Button(top, text="Disconnect",
                                        command=self._disconnect,
                                        bg="#e53935", fg="white",
                                        font=("Helvetica", 9, "bold"),
                                        relief="flat", padx=10,
                                        cursor="hand2", state="disabled")
        self.btn_disconnect.pack(side="left", padx=2)

        self.status_lbl = tk.Label(top, text="● Not connected",
                                   fg="#e53935", bg="#2b2b2b",
                                   font=("Helvetica", 9))
        self.status_lbl.pack(side="left", padx=12)

        # ── Toolbar with file operations
        toolbar = tk.Frame(self.root, bg="#2b2b2b")
        toolbar.pack(fill="x", padx=10, pady=5)
        
        self.btn_save = tk.Button(toolbar, text="💾 Save Current Scan",
                                  command=self._save_scan_data,
                                  bg="#4caf50", fg="white",
                                  font=("Helvetica", 9, "bold"),
                                  relief="flat", padx=10, cursor="hand2",
                                  state="disabled")
        self.btn_save.pack(side="left", padx=2)
        
        self.btn_load = tk.Button(toolbar, text="📂 Load & Plot Saved Scan",
                                  command=self._load_and_plot,
                                  bg="#2196f3", fg="white",
                                  font=("Helvetica", 9, "bold"),
                                  relief="flat", padx=10, cursor="hand2")
        self.btn_load.pack(side="left", padx=2)
        
        self.btn_clear = tk.Button(toolbar, text="🗑 Clear Plot",
                                   command=self._clear,
                                   bg="#ff9800", fg="white",
                                   font=("Helvetica", 9, "bold"),
                                   relief="flat", padx=10, cursor="hand2")
        self.btn_clear.pack(side="left", padx=2)

        # ── Middle: polar plot on left, readings on right
        mid = tk.Frame(self.root, bg="#2b2b2b")
        mid.pack(fill="both", expand=True, padx=10)

        # Polar plot
        self.fig = Figure(figsize=(6, 5), facecolor="#1e1e1e")
        self.ax = self.fig.add_subplot(111, projection="polar")
        self._reset_plot()

        self.canvas = FigureCanvasTkAgg(self.fig, master=mid)
        self.canvas.get_tk_widget().pack(side="left", fill="both", expand=True)
        
        # Add matplotlib toolbar for zoom/pan
        toolbar_frame = tk.Frame(mid)
        toolbar_frame.pack(side="top", fill="x")
        self.toolbar = NavigationToolbar2Tk(self.canvas, toolbar_frame)
        self.toolbar.update()

        # Right panel
        right = tk.Frame(mid, bg="#2b2b2b", width=220)
        right.pack(side="left", fill="y", padx=(10, 0))
        right.pack_propagate(False)

        tk.Label(right, text="How to scan:", bg="#2b2b2b", fg="#aaa",
                 font=("Helvetica", 8, "bold")).pack(anchor="w", pady=(0, 2))
        tk.Label(right, text="Type 's' below + Enter",
                 bg="#2b2b2b", fg="#4caf50",
                 font=("Helvetica", 8)).pack(anchor="w", pady=(0, 10))

        tk.Label(right, text="Readings", bg="#2b2b2b", fg="#aaa",
                 font=("Helvetica", 9, "bold")).pack(anchor="w")

        self.reading_box = tk.Text(right, bg="#1e1e1e", fg="#80cbc4",
                                   font=("Courier", 8), relief="flat",
                                   height=15, state="disabled")
        self.reading_box.pack(fill="both", expand=True)

        # ── Serial terminal
        term_frame = tk.Frame(self.root, bg="#2b2b2b")
        term_frame.pack(fill="x", padx=10, pady=(4, 0))

        tk.Label(term_frame, text="Serial Terminal",
                 bg="#2b2b2b", fg="#aaa",
                 font=("Helvetica", 8, "bold")).pack(anchor="w")

        self.log = scrolledtext.ScrolledText(term_frame, height=6,
                                             bg="#1e1e1e", fg="#a5d6a7",
                                             font=("Courier", 8),
                                             relief="flat", state="disabled")
        self.log.pack(fill="x")

        # ── Input row: text box + send button
        input_frame = tk.Frame(self.root, bg="#2b2b2b")
        input_frame.pack(fill="x", padx=10, pady=(4, 8))

        tk.Label(input_frame, text="Send:", bg="#2b2b2b", fg="#aaa",
                 font=("Helvetica", 9)).pack(side="left", padx=(0, 6))

        self.input_var = tk.StringVar()
        self.input_entry = tk.Entry(input_frame, textvariable=self.input_var,
                                    width=20, bg="#3c3c3c", fg="white",
                                    insertbackground="white",
                                    font=("Courier", 10), relief="flat")
        self.input_entry.pack(side="left", padx=(0, 6))
        self.input_entry.bind("<Return>", lambda e: self._send_input())

        self.btn_send = tk.Button(input_frame, text="Send",
                                  command=self._send_input,
                                  bg="#1976d2", fg="white",
                                  font=("Helvetica", 9, "bold"),
                                  relief="flat", padx=12, cursor="hand2",
                                  state="disabled")
        self.btn_send.pack(side="left")

        tk.Label(input_frame,
                 text="← type 's' here and press Enter to scan",
                 bg="#2b2b2b", fg="#666",
                 font=("Helvetica", 8)).pack(side="left", padx=10)

    # ── POLAR PLOT SETUP ───────────────────────────
    def _reset_plot(self):
        """Reset the polar plot with proper formatting"""
        self.ax.cla()
        self.ax.set_facecolor("#1e1e1e")
        self.ax.set_thetamin(0)
        self.ax.set_thetamax(180)
        self.ax.set_theta_zero_location("W")
        self.ax.set_theta_direction(-1)
        self.ax.set_rmax(2.5)  # Distance in meters
        self.ax.set_rticks([0.5, 1.0, 1.5, 2.0, 2.5])
        self.ax.tick_params(colors="#888", labelsize=8)
        self.ax.grid(color="#333", linewidth=0.6)
        self.ax.set_title("Distance (meters)", color="#ccc", fontsize=10, pad=10)
        
        # Set angle ticks at 45-degree increments
        self.ax.set_xticks(np.radians([0, 45, 90, 135, 180]))
        self.ax.set_xticklabels(['0°', '45°', '90°', '135°', '180°'])
        
        if hasattr(self, "canvas"):
            self.canvas.draw()

    def _update_plot(self, angles, distances):
        """Update the polar plot with new data"""
        self._reset_plot()

        if not angles:
            self.canvas.draw()
            return

        rads = [math.radians(a) for a in angles]
        self.ax.plot(rads, distances, color="#29b6f6",
                     linewidth=2.0, alpha=0.9, marker='o', markersize=3)
        self.ax.fill(rads, distances, color="#29b6f6", alpha=0.2)
        self.canvas.draw()

        # Update readings list
        self.reading_box.config(state="normal")
        self.reading_box.delete("1.0", "end")
        for a, d in zip(angles, distances):
            self.reading_box.insert("end", f"{a:3d}°  {d:6.2f} m\n")
        self.reading_box.config(state="disabled")
        
        # Store data for saving
        self.current_angles = angles
        self.current_distances = distances
        self.scan_data = list(zip(angles, distances))
        
        # Enable save button if we have data
        if angles:
            self.btn_save.config(state="normal")
        else:
            self.btn_save.config(state="disabled")

    # ── SAVE SCAN DATA TO FILE (matches the mock file format) ──
    def _save_scan_data(self):
        """Save the current scan data to a text file in the same format as mock-cybot-sensor-scan.txt"""
        if not self.current_angles:
            messagebox.showwarning("No Data", "No scan data to save")
            return
        
        file_path = filedialog.asksaveasfilename(
            defaultextension=".txt",
            initialfile="cybot-sensor-scan.txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if file_path:
            try:
                with open(file_path, 'w') as f:
                    f.write("Angle(Degrees)\tDistance(m)\n")
                    for angle, dist in zip(self.current_angles, self.current_distances):
                        f.write(f"{angle}\t\t{dist:.1f}\n")
                    f.write("END\n")
                
                self._log(f"[Data saved to: {os.path.basename(file_path)}]\r\n")
                messagebox.showinfo("Success", f"Data saved to:\n{file_path}")
                
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save data: {str(e)}")
                self._log(f"[ERROR saving data: {e}]\r\n")

    # ── LOAD AND PLOT SAVED SCAN DATA ──
    def _load_and_plot(self):
        """Load a saved scan data file and plot it like the reference example"""
        file_path = filedialog.askopenfilename(
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if not file_path:
            return
        
        try:
            angles = []
            distances = []
            
            with open(file_path, 'r') as f:
                # Skip header
                header = f.readline()
                self._log(f"[Loading file: {os.path.basename(file_path)}]\r\n")
                self._log(f"[Header: {header.strip()}]\r\n")
                
                # Read data until END
                for line in f:
                    line = line.strip()
                    if line == "END":
                        break
                    if line:
                        parts = line.split()
                        if len(parts) >= 2:
                            try:
                                angle = float(parts[0])
                                dist = float(parts[1])
                                angles.append(angle)
                                distances.append(dist)
                                self._log(f"[Loaded: {angle}° = {dist}m]\r\n")
                            except ValueError:
                                continue
            
            if angles and distances:
                self._update_plot(angles, distances)
                self._log(f"[Loaded {len(angles)} data points from file]\r\n")
                messagebox.showinfo("Load Success", f"Loaded {len(angles)} data points")
            else:
                messagebox.showwarning("No Data", "No valid data found in file")
                
        except Exception as e:
            messagebox.showerror("Load Error", f"Failed to load data: {str(e)}")
            self._log(f"[ERROR loading data: {e}]\r\n")

    # ── PORT AUTO-DETECT ───────────────────────────
    def _auto_detect_port(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        for p in ports:
            if "19" in p:
                self.port_var.set(p)
                return
        if ports:
            self.port_var.set(ports[0])

    # ── CONNECT / DISCONNECT ───────────────────────
    def _connect(self):
        port = self.port_var.get().strip()
        try:
            self.ser = serial.Serial(port, BAUD_RATE, timeout=1)
            self.running = True
            self.status_lbl.config(text=f"● Connected ({port})", fg="#4caf50")
            self.btn_connect.config(state="disabled")
            self.btn_disconnect.config(state="normal")
            self.btn_send.config(state="normal")
            self.input_entry.focus()
            self._log(f"[Connected to {port}]\r\n")
            threading.Thread(target=self._reader, daemon=True).start()
        except Exception as e:
            self._log(f"[ERROR: {e}]\r\n")
            messagebox.showerror("Connection Error", str(e))

    def _disconnect(self):
        self.running = False
        if self.ser:
            self.ser.close()
            self.ser = None
        self.status_lbl.config(text="● Not connected", fg="#e53935")
        self.btn_connect.config(state="normal")
        self.btn_disconnect.config(state="disabled")
        self.btn_send.config(state="disabled")
        self._log("[Disconnected]\r\n")

    # ── SEND INPUT ─────────────────────────────────
    def _send_input(self):
        if not self.ser:
            return
        text = self.input_var.get().strip()
        if text:
            self.ser.write(text.encode())
            self._log(f"[Sent: '{text}']\r\n")
            self.input_var.set("")

    # ── SERIAL READER THREAD ───────────────────────
    def _reader(self):
        buf = ""
        collecting = False
        angles = []
        distances = []

        while self.running:
            try:
                if self.ser.in_waiting:
                    raw = self.ser.read(self.ser.in_waiting)
                    text = raw.decode("utf-8", errors="replace")
                    self.root.after(0, self._log, text)

                    buf += text
                    lines = buf.split("\n")
                    buf = lines[-1]

                    for line in lines[:-1]:
                        line = line.strip()

                        if "SCAN_START" in line:
                            collecting = True
                            angles = []
                            distances = []
                            self.root.after(0, self._log, 
                                            "[GUI] Collecting scan data...\r\n")

                        # Parse the tab-separated format: "Angle\t\tDistance"
                        elif collecting and line and not line.startswith("Angle") and not line.startswith("---"):
                            # Check if it's the DATA format or the simple format
                            if "DATA:" in line:
                                try:
                                    parts = line[5:].split(",")
                                    a = int(parts[0])
                                    d = float(parts[1])
                                    angles.append(a)
                                    distances.append(d)
                                except:
                                    pass
                            elif "\t" in line and not line.startswith("SCAN"):
                                try:
                                    parts = line.split("\t")
                                    if len(parts) >= 2:
                                        a = int(parts[0])
                                        d = float(parts[1])
                                        angles.append(a)
                                        distances.append(d)
                                except:
                                    pass

                        elif "SCAN_END" in line and collecting:
                            collecting = False
                            self.root.after(0, self._update_plot, 
                                            list(angles), list(distances))
                            self.root.after(0, self._log, 
                                            f"[GUI] Plot updated — {len(angles)} points\r\n")
                else:
                    time.sleep(0.02)

            except Exception as e:
                if self.running:
                    self.root.after(0, self._log, f"[ERROR] {e}\r\n")
                break

    # ── CLEAR ──────────────────────────────────────
    def _clear(self):
        self._reset_plot()
        self.reading_box.config(state="normal")
        self.reading_box.delete("1.0", "end")
        self.reading_box.config(state="disabled")
        self.current_angles = []
        self.current_distances = []
        self.scan_data = []
        self.btn_save.config(state="disabled")
        self._log("[Plot cleared]\r\n")

    # ── LOG ────────────────────────────────────────
    def _log(self, text):
        self.log.config(state="normal")
        self.log.insert("end", text)
        self.log.see("end")
        self.log.config(state="disabled")


# ── ENTRY POINT ────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    app = CyBotGUI(root)
    root.mainloop()
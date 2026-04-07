"""
Plot CyBot Scan Data from File
==============================
Reads a text file containing scan data and creates a polar plot.
Works with data copied from Putty or saved from previous scans.
"""

import numpy as np
import matplotlib.pyplot as plt
import os

def plot_scan_data(filename):
    """
    Plot scan data from a text file
    
    Expected file format:
    Angle(Degrees)	Distance(m)
    0		2.5
    4		2.0
    ...
    END
    """
    
    # Initialize arrays
    angle_degrees = []
    distance = []
    
    # Check if file exists
    if not os.path.exists(filename):
        print(f"Error: File '{filename}' not found!")
        return
    
    # Read and parse the file
    with open(filename, 'r') as file:
        # Read header
        header = file.readline()
        print(f"Header: {header.strip()}")
        
        # Read data lines
        for line in file:
            line = line.strip()
            
            # Stop at END marker
            if line == "END":
                break
            
            # Skip empty lines
            if not line:
                continue
            
            # Split by whitespace (handles tabs or spaces)
            parts = line.split()
            if len(parts) >= 2:
                try:
                    angle = float(parts[0])
                    dist = float(parts[1])
                    angle_degrees.append(angle)
                    distance.append(dist)
                except ValueError:
                    print(f"Skipping invalid line: {line}")
    
    # Check if we got any data
    if not angle_degrees:
        print("No valid data found in file!")
        return
    
    print(f"Loaded {len(angle_degrees)} data points")
    print(f"Angle range: {min(angle_degrees)}° to {max(angle_degrees)}°")
    print(f"Distance range: {min(distance):.2f}m to {max(distance):.2f}m")
    
    # Convert degrees to radians for polar plot
    angle_radians = np.radians(angle_degrees)
    
    # Create the polar plot
    fig, ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(8, 8))
    
    # Plot the data
    ax.plot(angle_radians, distance, color='r', linewidth=2.0, marker='o', markersize=4)
    ax.fill(angle_radians, distance, alpha=0.3, color='red')
    
    # Configure the plot
    ax.set_title('CyBot Sensor Scan (0-180 degrees)', size=14, pad=20)
    ax.set_xlabel('Distance (m)', fontsize=12)
    ax.set_ylabel('Angle (degrees)', fontsize=12)
    ax.tick_params(axis='both', which='major', labelsize=10)
    
    # Set distance limits and ticks (meters)
    ax.set_rmax(2.5)
    ax.set_rticks([0.5, 1.0, 1.5, 2.0, 2.5])
    ax.set_rlabel_position(-22.5)
    
    # Set angle limits and ticks
    ax.set_thetamax(180)
    ax.set_xticks(np.radians([0, 45, 90, 135, 180]))
    ax.set_xticklabels(['0°', '45°', '90°', '135°', '180°'])
    
    # Add grid
    ax.grid(True)
    
    # Show the plot
    plt.tight_layout()
    plt.show()
    
    # Optionally save the plot
    save_option = input("\nSave the plot? (y/n): ").lower()
    if save_option == 'y':
        output_file = filename.replace('.txt', '_plot.png')
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Plot saved as: {output_file}")

def main():
    """Main function"""
    print("\n=== CyBot Scan Data Plotter ===\n")
    print("This script plots scan data from a text file.")
    print("Make sure your file is in the same directory as this script.\n")
    
    # Ask for filename
    filename = input("Enter the filename (e.g., my-scan-data.txt): ").strip()
    
    if not filename:
        filename = "my-scan-data.txt"
        print(f"Using default: {filename}")
    
    # Add .txt if no extension
    if '.' not in filename:
        filename += '.txt'
    
    # Plot the data
    plot_scan_data(filename)

if __name__ == "__main__":
    main()
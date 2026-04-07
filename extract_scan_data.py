"""
Extract Scan Data from Putty Log
================================
Reads a Putty log file and extracts just the scan data into a clean format.
"""

import re
import os

def extract_scan_from_log(log_filename, output_filename=None):
    """
    Extract scan data from a Putty log file
    
    Args:
        log_filename: Path to the Putty log file
        output_filename: Where to save extracted data (optional)
    """
    
    if not os.path.exists(log_filename):
        print(f"Error: File '{log_filename}' not found!")
        return None
    
    print(f"Reading log file: {log_filename}")
    
    # Read the entire log
    with open(log_filename, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Look for the scan data section
    # Assuming your CyBot outputs in the format:
    # Angle(Degrees)	Distance(m)
    # 0		2.5
    # 4		2.0
    # ...
    # END
    
    # Find the header line
    header_match = re.search(r'Angle\(Degrees\)\s+Distance\(m\)', content)
    if not header_match:
        print("Error: Could not find scan data header in log file!")
        return None
    
    # Extract from header to END
    start_pos = header_match.start()
    end_match = re.search(r'END\s*\n', content[start_pos:])
    
    if not end_match:
        print("Warning: No END marker found, taking all data from header")
        scan_section = content[start_pos:]
    else:
        scan_section = content[start_pos:start_pos + end_match.end()]
    
    print(f"Found scan data section:")
    print("-" * 50)
    print(scan_section[:500] + "..." if len(scan_section) > 500 else scan_section)
    print("-" * 50)
    
    # Clean up and save to output file
    if output_filename is None:
        output_filename = log_filename.replace('.log', '_scan.txt')
        if output_filename == log_filename:
            output_filename = 'extracted_scan_data.txt'
    
    with open(output_filename, 'w') as f:
        f.write(scan_section)
    
    print(f"\nExtracted scan data saved to: {output_filename}")
    return output_filename

def main():
    """Main function"""
    print("\n=== Extract CyBot Scan Data from Putty Log ===\n")
    
    # Ask for filename
    filename = input("Enter Putty log filename (e.g., cybot_scan_log.txt): ").strip()
    
    if not filename:
        filename = "cybot_scan_log.txt"
        print(f"Using default: {filename}")
    
    # Extract the scan data
    output_file = extract_scan_from_log(filename)
    
    if output_file:
        print("\nData extracted! You can now plot it with:")
        print(f"  python plot_my_scan.py")
        print(f"  (then enter '{output_file}' as the filename)")
        
        # Ask if they want to plot now
        plot_now = input("\nPlot now? (y/n): ").lower()
        if plot_now == 'y':
            # Import and call the plot function
            from plot_my_scan import plot_scan_data
            plot_scan_data(output_file)

if __name__ == "__main__":
    main()
import serial
import serial.tools.list_ports
import time
import re
from datetime import datetime

class WeighingScale:
    """
    Class to interface with RS232 weighing scale
    """
    
    def __init__(self, port=None, baudrate=9600, timeout=1):
        """
        Initialize the scale interface
        
        Args:
            port: Serial port (e.g., 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
            baudrate: Communication speed (default 9600)
            timeout: Read timeout in seconds
        """
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_connection = None
        self.port = port
        
    def list_available_ports(self):
        """List all available serial ports"""
        ports = serial.tools.list_ports.comports()
        available_ports = []
        
        print("Available serial ports:")
        for i, port in enumerate(ports):
            available_ports.append(port.device)
            print(f"  [{i}] {port.device} - {port.description}")
        
        return available_ports
    
    def connect(self, port=None):
        """
        Connect to the scale
        
        Args:
            port: Serial port to connect to
        """
        if port:
            self.port = port
            
        if not self.port:
            raise ValueError("No port specified")
        
        try:
            # Configure serial connection according to scale specs:
            # 9600 baud, 8 data bits, 1 stop bit, no parity (10 bits total)
            self.serial_connection = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout
            )
            
            print(f"Connected to scale on {self.port}")
            return True
            
        except serial.SerialException as e:
            print(f"Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the scale"""
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
            print("Disconnected from scale")
    
    def read_raw_data(self):
        """
        Read raw data from scale
        
        Returns:
            Raw string from scale, or None if no data
        """
        if not self.serial_connection or not self.serial_connection.is_open:
            print("Not connected to scale")
            return None
        
        try:
            # Read until newline or return character
            data = self.serial_connection.readline()
            
            if data:
                # Decode bytes to string, removing null bytes
                raw_string = data.decode('ascii', errors='ignore').strip()
                return raw_string
            
            return None
            
        except serial.SerialException as e:
            print(f"Error reading from scale: {e}")
            return None
    
    def parse_weight_data(self, raw_data):
        """
        Parse the raw data according to the scale format:
        Positions: 1:sign, 2:space, 3-9:data/., A-C:unit, D:end, E:return
        
        Args:
            raw_data: Raw string from scale
            
        Returns:
            Dictionary with parsed weight data
        """
        if not raw_data or len(raw_data) < 10:
            return None
        
        try:
            # Expected format: "+ 1234.56 kg\r\n"
            # Position 1: Sign (+ or -)
            sign = raw_data[0]
            
            # Position 2: Space (expected but flexible)
            # Position 3-9: Weight value with possible decimal points
            weight_str = raw_data[2:10].strip()
            
            # Positions A-C: Unit (3 characters)
            if len(raw_data) > 9:
                unit = raw_data[10:12].strip() if len(raw_data) >= 12 else raw_data[10:].strip()
            else:
                unit = ""
            
            # Clean the weight string and convert to float
            weight_str = weight_str.replace(' ', '')
            
            # Apply sign
            if sign == '-':
                weight_value = -float(weight_str)
            else:
                weight_value = float(weight_str)
            
            return {
                'weight': weight_value,
                'unit': unit,
                'raw': raw_data,
                'timestamp': datetime.now()
            }
            
        except (ValueError, IndexError) as e:
            print(f"Error parsing data '{raw_data}': {e}")
            return None
    
    def get_weight(self):
        """
        Get current weight reading
        
        Returns:
            Dictionary with weight data, or None
        """
        raw_data = self.read_raw_data()
        
        if raw_data:
            return self.parse_weight_data(raw_data)
        
        return None
    
    def wait_for_stable_reading(self, stability_threshold=0.001, samples=5, delay=0.1):
        """
        Wait for a stable weight reading
        
        Args:
            stability_threshold: Maximum variation for stability
            samples: Number of consecutive stable samples required
            delay: Delay between readings
        
        Returns:
            Stable weight data, or None
        """
        readings = []
        
        while len(readings) < samples:
            data = self.get_weight()
            
            if data:
                readings.append(data['weight'])
                
                if len(readings) > samples:
                    readings.pop(0)
                    
                    # Check stability
                    if max(readings) - min(readings) <= stability_threshold:
                        return data
                
                time.sleep(delay)
        
        return None


def continuous_monitor(scale, duration=None, save_to_file=False):
    """
    Continuously monitor and display weight readings
    
    Args:
        scale: WeighingScale instance
        duration: Duration in seconds (None for indefinite)
        save_to_file: Whether to save readings to a file
    """
    if save_to_file:
        filename = f"weight_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        file = open(filename, 'w')
        file.write("Timestamp,Weight,Unit\n")
        print(f"Saving data to {filename}")
    
    start_time = time.time()
    reading_count = 0
    
    print("\nContinuous monitoring started. Press Ctrl+C to stop.\n")
    print("-" * 50)
    
    try:
        while True:
            # Check duration
            if duration and (time.time() - start_time) > duration:
                break
            
            data = scale.get_weight()
            
            if data:
                reading_count += 1
                timestamp = data['timestamp'].strftime('%H:%M:%S.%f')[:-3]
                
                print(f"[{timestamp}] {data['weight']:>10.3f} {data['unit']}")
                
                if save_to_file:
                    file.write(f"{data['timestamp'].isoformat()},{data['weight']},{data['unit']}\n")
            
            time.sleep(0.1)  # Small delay to prevent CPU overuse
            
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user")
    
    finally:
        print("-" * 50)
        print(f"Total readings: {reading_count}")
        
        if save_to_file:
            file.close()
            print(f"Data saved to {filename}")
        
        scale.disconnect()


def interactive_mode(scale):
    """
    Interactive mode for manual weight readings
    """
    print("\nInteractive Mode")
    print("Press 'Enter' to read weight, 'q' to quit")
    
    while True:
        command = input("\n> ").strip().lower()
        
        if command == 'q':
            break
        
        data = scale.get_weight()
        
        if data:
            print(f"Weight: {data['weight']:.3f} {data['unit']}")
            print(f"Raw: {data['raw']}")
        else:
            print("No data received. Check connection and scale settings.")


def main():
    """Main program"""
    print("=" * 50)
    print("Weighing Scale RS232 Interface")
    print("=" * 50)
    
    # Create scale instance
    scale = WeighingScale()
    
    # List available ports
    ports = scale.list_available_ports()
    
    if not ports:
        print("\nNo serial ports found!")
        print("Please check connections and try again.")
        return
    
    # Select port
    try:
        port_index = int(input(f"\nSelect port [0-{len(ports)-1}]: "))
        selected_port = ports[port_index]
    except (ValueError, IndexError):
        print("Invalid selection")
        return
    
    # Connect to scale
    if not scale.connect(selected_port):
        return
    
    # Mode selection
    print("\nSelect mode:")
    print("  1. Continuous monitoring (for C5-0 continuous output)")
    print("  2. Interactive (press Enter to read)")
    print("  3. Single reading")
    
    try:
        mode = int(input("Mode [1-3]: "))
    except ValueError:
        mode = 3
    
    if mode == 1:
        save = input("Save to file? (y/n): ").lower() == 'y'
        try:
            duration = input("Duration in seconds (Enter for indefinite): ").strip()
            duration = float(duration) if duration else None
        except ValueError:
            duration = None
        
        continuous_monitor(scale, duration=duration, save_to_file=save)
        
    elif mode == 2:
        interactive_mode(scale)
        scale.disconnect()
        
    elif mode == 3:
        print("\nReading weight...")
        data = scale.get_weight()
        
        if data:
            print(f"\nWeight: {data['weight']:.3f} {data['unit']}")
            print(f"Raw data: {data['raw']}")
        else:
            print("Failed to get reading")
        
        scale.disconnect()


if __name__ == "__main__":
    main()
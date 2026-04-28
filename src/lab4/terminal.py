import serial
import time
import sys

PORT = '/dev/rfcomm0'
BAUDRATE = 115200
TIMEOUT = 1  # Response timeout in seconds

def main():
    try:
        # Open serial port
        ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
        print(f"Connected to {PORT}. Type 'exit' to quit.")

        while True:
            # Read command from user
            cmd = input(">> ")

            if cmd.lower() in ['exit', 'quit']:
                break

            # Send command + newline character
            full_cmd = cmd + "\n"
            ser.write(full_cmd.encode('utf-8'))

            # Wait and read response from MCU
            time.sleep(0.1) # Small pause for buffering
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting).decode('utf-8')
                print(response.strip())
            else:
                print("(No response)")

    except serial.SerialException as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Connection closed.")

if __name__ == "__main__":
    main()

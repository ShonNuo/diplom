import asyncio
import subprocess
from bleak import BleakScanner, BleakClient

# ESP32 name and characteristic UUIDs from firmware
DEVICE_NAME = "ESP32-BLE-Bridge"
CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

global_client = None

# Function triggered when a command is received from the Client
async def command_handler(sender, data):
    command = data.decode('utf-8', errors='ignore').strip()
    print(f"[+] Command received: '{command}'")

    try:
        # Execute the command in the OS terminal
        # shell=True allows running system commands like dir, echo, ipconfig
        process = subprocess.run(
            command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=10 # 10-second timeout to prevent hanging
        )

        # Collect output (stdout first, if empty — use stderr)
        output = process.stdout if process.stdout else process.stderr
        if not output:
            output = "[System]: Command executed, no output."

    except subprocess.TimeoutExpired:
        output = "[-] Error: Command timeout exceeded (10 sec)."
    except Exception as e:
        output = f"[-] Execution error: {str(e)}"

    print(f"[...] Sending response to client ({len(output)} bytes)...")

    # BLE transmits data in packets. If response is long, split into chunks (MTU)
    # Using safe default chunk size of 200 bytes
    chunk_size = 200
    output_bytes = output.encode('utf-8', errors='ignore')

    for i in range(0, len(output_bytes), chunk_size):
        chunk = output_bytes[i:i+chunk_size]
        if global_client and global_client.is_connected:
            await global_client.write_gatt_char(CHARACTERISTIC_UUID_TX, chunk)
            await asyncio.sleep(0.05) # Small delay between packets

async def main():
    global global_client
    print(f"[...] Searching for device {DEVICE_NAME}...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME)

    if not device:
        print(f"[-] Device {DEVICE_NAME} not found. Check if ESP32 is powered on.")
        return

    print(f"[+] ESP32 found: {device.address}. Connecting...")

    async with BleakClient(device) as client:
        global_client = client
        print("[+] Successfully connected to the bridge!")
        print("=== Server is running and waiting for commands ===")

        # Subscribe to TX characteristic (ESP32 forwards client commands here)
        # Since bleak requires a sync function in start_notify, we use asyncio.ensure_future
        await client.start_notify(
            CHARACTERISTIC_UUID_TX,
            lambda sender, data: asyncio.ensure_future(command_handler(sender, data))
        )

        # Keep the script running
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[-] Server stopped.")

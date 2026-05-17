import asyncio
from bleak import BleakScanner, BleakClient

# ESP32 name and characteristic UUIDs from firmware
DEVICE_NAME = "ESP32-BLE-Bridge"
CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

# Function to handle incoming responses from the server
def notification_handler(sender, data):
    print(f"\n[Response from Server]:\n{data.decode('utf-8', errors='ignore')}")
    print("\nEnter command: ", end="", flush=True)

async def main():
    print(f"[...] Searching for device {DEVICE_NAME}...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME)

    if not device:
        print(f"[-] Device {DEVICE_NAME} not found. Check if ESP32 is powered on.")
        return

    print(f"[+] ESP32 found: {device.address}. Connecting...")

    async with BleakClient(device) as client:
        print("[+] Successfully connected to the bridge!")

        # Subscribe to notifications to receive responses from the server
        await client.start_notify(CHARACTERISTIC_UUID_RX, notification_handler)

        print("\n=== Client is ready ===")
        print("Example commands: 'dir', 'echo Hello', 'ipconfig' (or 'ifconfig' for Linux)")
        print("Type 'exit' to quit")

        while True:
            # Read command from console (run in a thread to avoid blocking async loop)
            command = await asyncio.to_thread(input, "Enter command: ")

            if command.strip().lower() == 'exit':
                break

            if not command.strip():
                continue

            # Send command to the server via ESP32
            print(f"[...] Sending: '{command}'")
            await client.write_gatt_char(CHARACTERISTIC_UUID_RX, command.encode('utf-8'))

            # Small delay to allow processing and response output
            await asyncio.sleep(0.5)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[-] Client stopped.")

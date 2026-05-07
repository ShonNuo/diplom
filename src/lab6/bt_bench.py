#!/usr/bin/env python3
"""
Bluetooth Throughput Benchmark for ESP32 (Arduino IDE)
Поддерживает: Classic BT (SPP/RFCOMM) и BLE GATT
Требует: pip install bleak
"""
import sys
import time
import socket
import asyncio
import argparse
from bleak import BleakClient

# ─────────────────────────────────────────────────────────────
# КОНФИГУРАЦИЯ
# ─────────────────────────────────────────────────────────────
TEST_SIZE = 1024 * 1024 * 10          # 10 MB точный объём теста
BLE_CHAR  = "00002a56-0000-1000-8000-00805f9b34fb"
BYTES = 64 * 1024
PATTERN   = b"\xAA" * BYTES       # Чанк для отправки

# ─────────────────────────────────────────────────────────────
# CLASSIC BT (SPP / RFCOMM)
# ─────────────────────────────────────────────────────────────
def measure_classic(mac: str, direction: str):
    print(f"[Classic] Connecting to {mac}...")
    s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
    s.settimeout(5.0)
    
    try:
        s.connect((mac, 1))  # Если sdptool показал другой канал, замените 1
    except Exception as e:
        print(f"❌ Connection failed: {e}")
        print("💡 Проверьте канал: sdptool browse " + mac + " | grep Channel")
        return

    print("[Classic] Connected. Waiting for ESP BT stack to initialize...")
    time.sleep(1.5)  # КРИТИЧНО: даём Arduino-стеку перевести соединение в ready
    
    print(f"[Classic] Sending '{direction}' command...")
    s.send(direction.encode())
    time.sleep(0.1)

    if direction == "RX":  # PC -> ESP
        print(f"[Classic] PC -> ESP | Sending {TEST_SIZE} bytes...")
        t0 = time.perf_counter()
        sent = 0
        while sent < TEST_SIZE:
            to_send = min(len(PATTERN), TEST_SIZE - sent)
            s.send(PATTERN[:to_send])
            sent += to_send
        t1 = time.perf_counter()
        transferred = sent
    else:  # PC <- ESP
        print(f"[Classic] PC <- ESP | Waiting for {TEST_SIZE} bytes...")
        received = 0
        t0 = time.perf_counter()
        try:
            while received < TEST_SIZE:
                chunk = s.recv(BYTES)
                if not chunk:
                    print("❌ ESP closed connection unexpectedly")
                    break
                received += len(chunk)
                if received % (100*1024) == 0:
                    print(f"   ⏳ Received: {received//1024} KB / {TEST_SIZE//1024} KB")
            t1 = time.perf_counter()
            transferred = received
        except socket.timeout:
            print(f"❌ Timeout. Received {received//1024} KB in 30s. Check ESP Serial Monitor for errors.")
            transferred = received
            if transferred == 0:
                return  # Не считаем скорость, если данные не пошли

    elapsed = t1 - t0
    if elapsed <= 0: elapsed = 1e-6
    kbps = (transferred * 8) / elapsed / 1000
    kbs  = transferred / elapsed / 1024
    print(f"✅ Classic {direction} Done: {kbps:.2f} Kbps ({kbs:.2f} KB/s) | {elapsed:.3f}s\n")
    s.close()
# ─────────────────────────────────────────────────────────────
# BLE GATT SERVER
# ─────────────────────────────────────────────────────────────
async def measure_ble(mac: str, direction: str):
    print(f"[BLE] Connecting to {mac}...")
    try:
        async with BleakClient(mac) as client:
            mtu = getattr(client, "mtu_size", 512)
            print(f"[BLE] Connected. MTU: {mtu}")
            
            print(f"[BLE] Sending '{direction}' command...")
            await client.write_gatt_char(BLE_CHAR, direction.encode(), response=True)
            await asyncio.sleep(0.15)

            if direction == "RX":
                # PC → ESP (Write Requests)
                print(f"[BLE] PC -> ESP | Sending {TEST_SIZE} bytes...")
                chunk_size = max(20, mtu - 3)
                payload = PATTERN * (TEST_SIZE // len(PATTERN) + 1)
                
                t0 = time.perf_counter()
                for i in range(0, TEST_SIZE, chunk_size):
                    await client.write_gatt_char(BLE_CHAR, payload[i:i+chunk_size], response=True)
                t1 = time.perf_counter()
                transferred = TEST_SIZE

            else:
                # PC ← ESP (Notifications)
                print(f"[BLE] PC <- ESP | Waiting for {TEST_SIZE} bytes...")
                received = bytearray()
                t0 = None

                def notify_handler(sender, data):
                    nonlocal t0, received
                    if t0 is None: t0 = time.perf_counter()
                    received.extend(data)

                await client.start_notify(BLE_CHAR, notify_handler)
                while len(received) < TEST_SIZE:
                    await asyncio.sleep(0.01)
                t1 = time.perf_counter()
                await client.stop_notify(BLE_CHAR)
                transferred = len(received)

            elapsed = t1 - t0
            if elapsed <= 0: elapsed = 1e-6
            kbps = (transferred * 8) / elapsed / 1000
            kbs  = transferred / elapsed / 1024
            print(f"✅ BLE {direction} Done: {kbps:.2f} Kbps ({kbs:.2f} KB/s) | {elapsed:.3f}s\n")

    except Exception as e:
        print(f"❌ BLE Error: {e}")

# ─────────────────────────────────────────────────────────────
# CLI & RUN
# ─────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="ESP32 Bluetooth Throughput Benchmark")
    parser.add_argument("--mac", required=True, help="MAC-адрес ESP (например, AA:BB:CC:DD:EE:01)")
    parser.add_argument("--mode", choices=["classic_tx", "classic_rx", "ble_tx", "ble_rx", "all"], required=True)
    args = parser.parse_args()

    mac = args.mac.upper()
    print("🚀 Bluetooth Benchmark Starting...\n")

    if args.mode in ["classic_tx", "all"]:
        measure_classic(mac, "RX")  # PC шлёт → ESP принимает (со стороны ESP это RX)
    if args.mode in ["classic_rx", "all"]:
        measure_classic(mac, "TX")  # PC принимает ← ESP шлёт
    if args.mode in ["ble_tx", "all"]:
        asyncio.run(measure_ble(mac, "RX"))
    if args.mode in ["ble_rx", "all"]:
        asyncio.run(measure_ble(mac, "TX"))

    print("📊 Benchmark finished.")

if __name__ == "__main__":
    main()

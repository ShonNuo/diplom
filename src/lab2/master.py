import socket
import time
import bluetooth

# MAC addresses of slave devices (replace with real ones)
ESP1_MAC = "68:FE:71:12:E4:E2"
ESP2_MAC = "88:57:21:6A:0B:8A"
RFCOMM_PORT = 1  # Standard port for SPP

def connect_rfcomm(mac_addr, port=RFCOMM_PORT):
    sock = socket.socket(socket.AF_BLUETOOTH, 
                         socket.SOCK_STREAM, 
                         socket.BTPROTO_RFCOMM)
    try:
        sock.connect((mac_addr, port))
        print(f"[+] Connected to {mac_addr}")
        return sock
    except Exception as e:
        print(f"[-] Connection error to {mac_addr}: {e}")
        return None

def exchange_data(sock, device_id):
    if not sock: return
    #msg = f"Data from Master -> {device_id}\n"
    msg = "PING"
    sock.send(msg.encode('utf-8'))
    response = sock.recv(1024).decode('utf-8').strip()
    print(f"<- Response from {device_id}: {response}")

def main():
    print("=== Piconet initialization ===")
    s1 = connect_rfcomm(ESP1_MAC)
    s2 = connect_rfcomm(ESP2_MAC)
    
    if s1 and s2:
        print("=== Both channels active. Starting exchange ===")
        for i in range(5):
            exchange_data(s1, "ESP32_1")
            exchange_data(s2, "ESP32_2")
            time.sleep(1)
        
        s1.close()
        s2.close()
        print("=== Connections closed ===")
    else:
        print("[-] Failed to establish all connections")

if __name__ == "__main__":
    main()

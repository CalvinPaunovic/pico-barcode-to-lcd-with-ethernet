import socket

# Server-IP: feste IP deines PCs im selben Subnetz
SERVER_IP = "192.168.0.86"
SERVER_PORT = 4000

# UDP-Socket erstellen
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((SERVER_IP, SERVER_PORT))
print(f"UDP Server läuft auf {SERVER_IP}:{SERVER_PORT}, wartet auf Pico...")

try:
    while True:
        data, addr = server_socket.recvfrom(1024)  # max. 1024 Bytes
        print(f"Empfangen von {addr}: {data.decode()}")
        # Optional: Bestätigung zurück an den Pico senden
        server_socket.sendto(b"ACK\n", addr)

except KeyboardInterrupt:
    print("UDP Server wird beendet...")

finally:
    server_socket.close()
    print("Server geschlossen.")

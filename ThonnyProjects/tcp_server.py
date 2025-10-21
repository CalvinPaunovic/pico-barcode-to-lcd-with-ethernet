import socket

# Server-IP: die feste IP deines PCs im selben Subnetz
SERVER_IP = "172.16.28.240"
SERVER_PORT = 5000

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((SERVER_IP, SERVER_PORT))  # feste IP
server_socket.listen(1)
print(f"TCP Server läuft auf {SERVER_IP}:{SERVER_PORT}, wartet auf Pico...")

try:
    conn, addr = server_socket.accept()
    print("Verbindung hergestellt von:", addr)

    while True:
        data = conn.recv(1024)
        if not data:
            print("Verbindung beendet.")
            break
        print("Empfangen vom Pico:", data.decode())
        conn.send(b"ACK\n")

except KeyboardInterrupt:
    print("Server wird beendet...")

finally:
    conn.close()
    server_socket.close()
    print("Server geschlossen.")

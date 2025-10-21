import socket
import time

# IP und Port des Servers
SERVER_IP = "172.16.28.240"
SERVER_PORT = 5000

# TCP-Socket erstellen
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    client_socket.connect((SERVER_IP, SERVER_PORT))
    print(f"Verbunden mit Server {SERVER_IP}:{SERVER_PORT}")

    while True:
        message = input("Nachricht an Server: ")
        if not message:
            break
        client_socket.send(message.encode())

        # Antwort vom Server empfangen
        response = client_socket.recv(1024)
        print("Antwort vom Server:", response.decode().strip())

except KeyboardInterrupt:
    print("\nClient wird beendet...")

except Exception as e:
    print("Fehler:", e)

finally:
    client_socket.close()
    print("Verbindung geschlossen.")

import socket
import mysql.connector
from datetime import datetime

# MySQL Verbindung herstellen
print("Verbinde mit MySQL...")
conn = mysql.connector.connect(
    host="localhost",
    port=3306,
    user="root",
    password="cpaun2022",
    database="ch9121_test"
)
cursor = conn.cursor()
print("MySQL Verbindung erfolgreich\n")

# TCP Server erstellen
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(('0.0.0.0', 5000))  # Port 5000, lauscht auf allen Interfaces
server_socket.listen(1)

print("=" * 50)
print("MySQL Bridge Server gestartet")
print("Port: 5000")
print("Warte auf Verbindung vom Pico...")
print("=" * 50)

while True:
    client_socket, address = server_socket.accept()
    print(f"\nVerbindung von {address}")
    
    buffer = ""
    
    try:
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            
            # Daten zum Buffer hinzufügen
            buffer += data.decode('utf-8', errors='ignore')
            
            # Verarbeite alle vollständigen Zeilen (getrennt durch \n)
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                barcode = line.strip()
                
                if barcode:
                    try:
                        sql = "INSERT INTO scanned_barcodes (barcode, timestamp) VALUES (%s, %s)"
                        values = (barcode, datetime.now())
                        cursor.execute(sql, values)
                        conn.commit()
                        print(f"  Barcode eingefügt: {barcode}")
                    except mysql.connector.Error as err:
                        print(f"  MySQL Fehler: {err}")

    except Exception as e:
        print(f"Verbindungsfehler: {e}")
    
    finally:
        client_socket.close()
        print(f"Verbindung von {address} getrennt")
        print("Warte auf neue Verbindung...\n")

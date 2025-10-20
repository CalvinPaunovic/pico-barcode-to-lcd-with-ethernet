import mysql.connector
from datetime import datetime

conn = mysql.connector.connect(
    host="localhost",
    port=3306,
    user="root",
    password="cpaun2022",
    database="ch9121_test"
)

cursor = conn.cursor()

barcode = input("Barcode eingeben: ").strip()

if not barcode:
    print("Fehler: Kein Barcode eingegeben.")
else:
    sql = "INSERT INTO scanned_barcodes (barcode, timestamp) VALUES (%s, %s)"
    values = (barcode, datetime.now())
    cursor.execute(sql, values)
    conn.commit()
    print(f"{cursor.rowcount} Datensatz eingefügt.")

cursor.close()
conn.close()

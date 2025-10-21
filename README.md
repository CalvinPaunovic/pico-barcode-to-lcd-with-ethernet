Projekt für die Kommunikation zwischen Barcode-Scanner, LCD 1602 I2C (PCF8574-Chip!) und CH9121-Modul 
sowie Einpflegen von den gescannten Barcodes in eine MySQL-Datenbank.

- Getestet für den Raspberry Pi Pico 2 WH
- Zusammengesetzt aus den Example-Projects der Raspberry Pi Pico Extension für VS-Code: 'picow_blink', 'host_cdc_msc_hid', 'lcd_1602_i2c',
  sowie https://github.com/Danielerikrust/CH9121 und https://www.waveshare.com/wiki/Pico-ETH-CH9121 

SETUP-ANLEITUNG:

1) Bootsel-Taste des Picos gedrückt halten, dann an den PC anschließen und loslassen, 
   um in den Bootloader-Modus zu gelangen.
   
2) Rechtsklick auf CMakeLists.txt im Projekt-Explorer -> "Clean Rebuild all Projects" 
   -> entstehende .uf2 Datei vom build-Ordner auf den Pico flashen.
   Der Pico sollte nun automatisch neustarten.
   
3) Datenverbindung mit dem PC trennen und Pico nun über VBUS (Pin 40) und GND (Pin 3) 
   mit Strom versorgen.
   
4) Barcode-Scanner über Micro-USB anschließen und CH9121-Modul mit dem Header verbinden.

5) LCD 1602 I2C Modul an Raspberry Pi Pico anschließen:
   GPIO 4 (Pin 6)  -> SDA am LCD Bridge Board
   GPIO 5 (Pin 7)  -> SCL am LCD Bridge Board
   3.3V (Pin 36)   -> VCC am LCD Bridge Board
   GND (Pin 38)    -> GND am LCD Bridge Board
   
6) Modus des CH9121 unter 'ch9121_config' einstellen: 
   TCP_CLIENT, TCP_SERVER, UDP_CLIENT, UDP_SERVER
   
7) IP-Adresse und Ports unter 'ch9121_config' anpassen 
   (local_ip, gateway, subnet_mask, target_ip)
   
8) Zwischenserver mysql_bridge.py gemäß der MySQL-Datenbank konfigurieren 
   und mit 'python mysql_bridge.py' starten.
   
9) Barcode-Scanner einschalten und Barcodes scannen. Die Barcodes sollten auf dem LCD 
   angezeigt werden und an den Zwischenserver gesendet werden, der sie in die 
   MySQL-Datenbank einfügt.

HINWEIS: Level-Shifter werden für die I2C-Leitungen empfohlen, 
         wenn das Board mit 5V betrieben wird.

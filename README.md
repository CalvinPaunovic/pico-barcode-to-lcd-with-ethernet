Finales Projekt befindet sich im Ordner pico-barcode-to-lcd-with-ethernet.

- Programmierumgebung: Visual Studio Code
- Extensions: Raspberry Pi Pico unter Visual Studio Code (Liefert die vollständige pico-sdk mit Submodule)
- Hardware:
    - Raspberry Pi Pico 2 WH
    - Waveshare Barcode-Scanner-Modul (https://www.waveshare.com/wiki/Barcode_Scanner_Module)
    - Waveshare ch9121 (https://www.waveshare.com/pico-eth-ch9121.htm) (mit LAN-Kabel)
    - I2C LCD 1602 mit PCF8574-Chip (mit vier Jumper-Kabeln)
    - OTG-Kabel für die Verbindung zwischen Barcode-Scanner und Pico

Projekt für die Kommunikation zwischen Barcode-Scanner, LCD und CH9121-Modul 
sowie Einpflegen von den gescannten Barcodes in eine MySQL-Datenbank.

- Getestet für den Raspberry Pi Pico 2 WH
- Zusammengesetzt aus den Example-Projects der Raspberry Pi Pico Extension für VS-Code:
    - 'picow_blink' (LED des Picos ansteuern)
    - 'host_cdc_msc_hid' (tinyUSB-Bibliothek, um den freien Micro-USB als Host zu konfigurieren)
    - 'lcd_1602_i2c' (Kommunikation mit dem LCD)
    - sowie https://github.com/Danielerikrust/CH9121 und https://www.waveshare.com/wiki/Pico-ETH-CH9121 (ch9121-Modul konfigurieren).

SETUP-ANLEITUNG:

1) Modus des CH9121 unter 'ch9121_config' in der main.c anpassen: 
   TCP_CLIENT, TCP_SERVER, UDP_CLIENT, UDP_SERVER
   
2) IP-Adresse und Ports unter 'ch9121_config' in der main.c einstellen: 
   (local_ip, gateway, subnet_mask, target_ip)

3) Bootsel-Taste des Picos gedrückt halten, dann an den PC anschließen und loslassen, 
   um in den Bootloader-Modus zu gelangen.
   
4) Rechtsklick auf CMakeLists.txt im Projekt-Explorer -> "Clean Rebuild all Projects" 
   -> entstehende .uf2 Datei vom build-Ordner auf den Pico flashen.
   Der Pico sollte nun automatisch neustarten.
   
5) Datenverbindung mit dem PC trennen und Pico nun über VBUS (Pin 40) und GND (Pin 3) 
   mit Strom versorgen.
   
6) Barcode-Scanner über Micro-USB anschließen und CH9121-Modul mit dem Header verbinden.

7) LCD 1602 I2C Modul an Raspberry Pi Pico anschließen:
   - GPIO 4 (Pin 6)  -> SDA am LCD Bridge Board
   - GPIO 5 (Pin 7)  -> SCL am LCD Bridge Board
   - 3.3V (Pin 36)   -> VCC am LCD Bridge Board
   - GND (Pin 38)    -> GND am LCD Bridge Board
   
8) Zwischenserver mysql_bridge.py gemäß der MySQL-Datenbank konfigurieren 
   und mit 'python mysql_bridge.py' starten.
   
9) Barcode-Scanner einschalten und Barcodes scannen. Die Barcodes sollten auf dem LCD 
   angezeigt werden und an den Zwischenserver gesendet werden, der sie in die 
   MySQL-Datenbank einfügt.

HINWEIS: Level-Shifter werden für die I2C-Leitungen empfohlen, 
         wenn das Board mit 5V betrieben wird.

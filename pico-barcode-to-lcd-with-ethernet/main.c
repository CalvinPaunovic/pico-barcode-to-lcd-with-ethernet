#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "pico/cyw43_arch.h"

#include "CH9121.h"
#include "lcd_1602_i2c.h"

// UART0 is used for data communication with CH9121 (network traffic)
// UART0 wird für die Datenkommunikation mit dem CH9121-Modul verwendet (Netzwerkverkehr)
#define UART_ID0        uart0
#define UART_TX_PIN0    0
#define UART_RX_PIN0    1

// Funktionsdeklarationen für LED-Steuerung und HID-Verarbeitung
void hid_app_task(void);
void led_request_off_ms(uint32_t ms);
void led_service(void);
void cyw43_led_init(void);

/*
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
*/

// CH9121 Konfiguration (anpassen)
// Konfigurationsstruktur für das CH9121 Ethernet-Modul
// Enthält Netzwerkparameter wie IP-Adressen, Ports, Baudrate und Betriebsmodus
CH9121_Config ch9121_config = {
        .local_ip     = {172, 16, 28, 241},  // Lokale IP-Adresse des CH9121-Moduls
        .gateway      = {172, 16, 28, 1},     // Gateway-Adresse des Netzwerks
        .subnet_mask  = {255, 255, 255, 0},   // Subnetzmaske
        .target_ip    = {172, 16, 28, 240},   // Ziel-IP-Adresse (Server)
        .local_port   = 4000,                 // Lokaler Port für die Kommunikation
        .target_port  = 5000,                 // Zielport auf dem Server
        .baud_rate    = 115200,               // UART-Baudrate für CH9121-Kommunikation
        .mode         = TCP_CLIENT            // Betriebsmodus: TCP-Client
};

// LED Deadline Logic
// Globale Variable für die LED-Ausschaltverzögerung (in Millisekunden)
static uint32_t g_led_off_until = 0;

// Schaltet die eingebaute LED ein
static inline void led_on(void)  { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); }

// Schaltet die eingebaute LED aus
static inline void led_off(void) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); }

// Fordert das Ausschalten der LED für eine bestimmte Anzahl von Millisekunden an
// Parameter ms: Anzahl der Millisekunden, für die die LED ausgeschaltet bleiben soll
void led_request_off_ms(uint32_t ms) { g_led_off_until = board_millis() + ms; led_off(); }

// Service-Funktion für die LED-Steuerung
// Überprüft, ob die LED-Ausschaltverzögerung abgelaufen ist und schaltet die LED entsprechend ein oder aus
void led_service(void) { uint32_t now = board_millis(); if ((int32_t)(now - g_led_off_until) < 0) led_off(); else led_on(); }

// Netzwerk-Senden über UART0 -> CH9121
// Sendet eine Textzeile über UART0 an das CH9121-Modul zur Weiterleitung über Ethernet
// Parameter line: Null-terminierter String, der gesendet werden soll
void net_send_line(const char* line) {
        uart_puts(UART_ID0, line);
}

// Initialisiert das CYW43-Modul und die LED
// Das CYW43-Modul steuert die integrierte LED auf dem Pico W/WH
void cyw43_led_init(void) {
    if (cyw43_arch_init()) {
        printf("CYW43 init failed\r\n");
    } else {
        // LED initial ausschalten
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
}

// Hauptfunktion des Programms
// Initialisiert alle Komponenten und startet die Hauptschleife
int main(void) {
    // Board/TinyUSB
    // Initialisiert die Board-spezifischen Funktionen und Standard-I/O
    board_init();
    stdio_init_all();

    printf("USB-HID Barcode -> LCD + Ethernet (CH9121)\r\n");

    // Initialisiert die CYW43-LED
    cyw43_led_init();
    
    // Initialisiert das LCD 1602 I2C-Display
    lcd_1602_i2c_init();

    // TinyUSB Host initialisieren
    // Ermöglicht dem Pico, als USB-Host für den Barcode-Scanner zu fungieren
    tuh_init(BOARD_TUH_RHPORT);
    if (board_init_after_tusb) { board_init_after_tusb(); }

    // CH9121 konfigurieren
    // Sendet die Konfigurationsparameter an das CH9121-Modul
    CH9121_configure(&ch9121_config);

    // UART0 für Datenverkehr mit CH9121
    // Initialisiert UART0 mit der konfigurierten Baudrate
    uart_init(UART_ID0, ch9121_config.baud_rate);
    
    // Setzt die GPIO-Pins für UART-Funktionalität
    gpio_set_function(UART_TX_PIN0, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN0, GPIO_FUNC_UART);

    // kurze Wartezeit
    // Gibt dem System Zeit, alle Initialisierungen abzuschließen
    sleep_ms(500);

    // Hauptschleife
    // Führt kontinuierlich die erforderlichen Tasks aus
    while (1) {
        // TinyUSB Host Polling
        // Verarbeitet USB-Host-Events (z.B. HID-Reports vom Barcode-Scanner)
        tuh_task();
        
        // HID App (Barcode Verarbeitung)
        // Verarbeitet empfangene Barcode-Daten
        hid_app_task();
        
        // LED Service
        // Aktualisiert den LED-Status basierend auf der Verzögerungslogik
        led_service();
    }
}

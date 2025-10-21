// Vereinfachter 16x2 LCD I2C Treiber (PCF8574 Backpack)
// Hinweise: basiert lose auf dem Beispiel lcd_1602_i2c.c, aber stark reduziert.
//
// Dieser Treiber implementiert die Ansteuerung eines 16x2 LCD-Displays über I2C
// mittels eines PCF8574 I2C-zu-Parallel-Wandler-Chips (LCD-Backpack).

#include "lcd_1602_i2c.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>


/* Example code to drive a 16x2 LCD panel via a I2C bridge chip (e.g. PCF8574)

   NOTE: The panel must be capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefore I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO 4 (pin 6)-> SDA on LCD bridge board
   GPIO 5 (pin 7)-> SCL on LCD bridge board
   3.3v (pin 36) -> VCC on LCD bridge board
   GND (pin 38)  -> GND on LCD bridge board
*/

// I2C-Instanz, die für die LCD-Kommunikation verwendet wird
#ifndef LCD_I2C_INSTANCE
#define LCD_I2C_INSTANCE i2c0
#endif

// GPIO-Pin für I2C SDA (Data Line)
#ifndef LCD_I2C_SDA_PIN
#define LCD_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN
#endif

// GPIO-Pin für I2C SCL (Clock Line)
#ifndef LCD_I2C_SCL_PIN
#define LCD_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN
#endif

// I2C-Adresse des PCF8574-Chips (Standard für viele LCD-Module)
static int lcd_addr = 0x27;

// Kommandos
// HD44780-kompatible LCD-Befehle
#define LCD_CLEARDISPLAY 0x01    // Löscht das Display und setzt Cursor auf Position 0
#define LCD_RETURNHOME   0x02    // Setzt Cursor auf Position 0
#define LCD_ENTRYMODESET 0x04    // Setzt den Entry-Modus (Cursor-Bewegungsrichtung)
#define LCD_DISPLAYCONTROL 0x08  // Display ein/aus, Cursor ein/aus, Blinken ein/aus
#define LCD_FUNCTIONSET  0x20    // Setzt Interface-Länge, Zeilenanzahl, Zeichengröße

// Flags
// Flags für LCD-Befehle
#define LCD_ENTRYLEFT    0x02    // Entry-Modus: Cursor bewegt sich nach rechts
#define LCD_DISPLAYON    0x04    // Display einschalten
#define LCD_2LINE        0x08    // 2-Zeilen-Modus

// PCF8574 Pin-Belegung für LCD-Steuerung
#define LCD_BACKLIGHT    0x08    // Backlight-Bit (Pin 3 des PCF8574)
#define LCD_ENABLE_BIT   0x04    // Enable-Bit (Pin 2 des PCF8574)

// Modus-Flags für Datenübertragung
#define LCD_MODE_CHAR 1          // Modus für Zeichen (RS=1)
#define LCD_MODE_CMD  0          // Modus für Befehle (RS=0)

// Schreibt ein einzelnes Byte über I2C an das LCD-Modul
// Parameter val: Zu sendendes Byte (kombiniert Daten und Steuersignale)
static void lcd_write_raw(uint8_t val) {
    i2c_write_blocking(LCD_I2C_INSTANCE, lcd_addr, &val, 1, false);
}

// Erzeugt einen Enable-Puls für das LCD
// Parameter val: Aktueller Datenwert
// 
// Das LCD benötigt einen Enable-Puls (high-low) um Daten zu übernehmen.
// Timing gemäß HD44780-Spezifikation: >450ns für Enable high, >450ns für Enable low
static void lcd_toggle(uint8_t val) {
    sleep_us(600);                               // Warte vor Enable-Puls
    lcd_write_raw(val | LCD_ENABLE_BIT);         // Enable high
    sleep_us(600);                               // Enable high halten
    lcd_write_raw(val & ~LCD_ENABLE_BIT);        // Enable low
    sleep_us(600);                               // Warte nach Enable-Puls
}

// Sendet ein 8-Bit-Wert im 4-Bit-Modus an das LCD
// Parameter value: Zu sendendes Byte
// Parameter mode: LCD_MODE_CMD für Befehle, LCD_MODE_CHAR für Zeichen
// 
// Im 4-Bit-Modus werden 8 Bit in zwei 4-Bit-Nibbles gesendet:
// Zuerst die oberen 4 Bits, dann die unteren 4 Bits
static void lcd_send(uint8_t value, uint8_t mode) {
    // Obere 4 Bits: kombiniert mit Mode-Flag und Backlight
    uint8_t high = mode | (value & 0xF0) | LCD_BACKLIGHT;
    // Untere 4 Bits: nach links verschoben, kombiniert mit Mode-Flag und Backlight
    uint8_t low  = mode | ((value << 4) & 0xF0) | LCD_BACKLIGHT;
    
    lcd_write_raw(high); lcd_toggle(high);       // Sendet oberes Nibble
    lcd_write_raw(low);  lcd_toggle(low);        // Sendet unteres Nibble
}

// Sendet einen Befehl an das LCD
// Parameter cmd: LCD-Befehlsbyte
static void lcd_command(uint8_t cmd) { lcd_send(cmd, LCD_MODE_CMD); }

// Sendet ein Zeichen an das LCD
// Parameter c: ASCII-Zeichen
static void lcd_char(uint8_t c) { lcd_send(c, LCD_MODE_CHAR); }

// Löscht das gesamte Display und setzt den Cursor auf Position (0,0)
void lcd_1602_i2c_clear(void) { lcd_command(LCD_CLEARDISPLAY); sleep_ms(2); }

// Initialisiert das LCD-Display
// Führt die Initialisierungssequenz gemäß HD44780-Spezifikation durch
void lcd_1602_i2c_init(void) {
    // I2C Setup (100kHz ausreichend)
    // Initialisiert die I2C-Schnittstelle mit 100 kHz
    i2c_init(LCD_I2C_INSTANCE, 100 * 1000);
    
    // Konfiguriert die GPIO-Pins für I2C-Funktion
    gpio_set_function(LCD_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    // Aktiviert Pull-up-Widerstände für I2C-Leitungen
    gpio_pull_up(LCD_I2C_SDA_PIN);
    gpio_pull_up(LCD_I2C_SCL_PIN);

    // Init Sequenz
    // Initialisierungssequenz nach Power-On (mind. 40ms warten)
    sleep_ms(50);
    
    // Sendet dreimal 0x03 im 8-Bit-Modus (Soft-Reset-Sequenz)
    lcd_command(0x03); sleep_ms(5);
    lcd_command(0x03); sleep_ms(5);
    lcd_command(0x03); sleep_ms(5);
    
    // Wechselt in den 4-Bit-Modus
    lcd_command(0x02); // 4-bit

    // Konfiguriert das LCD: 4-Bit-Interface, 2 Zeilen, 5x8 Zeichen
    lcd_command(LCD_FUNCTIONSET | LCD_2LINE);
    
    // Setzt Entry-Modus: Cursor bewegt sich nach rechts, kein Display-Shift
    lcd_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
    
    // Schaltet Display ein, Cursor aus, Blinken aus
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
    
    // Löscht das Display
    lcd_1602_i2c_clear();
}

// Setzt den Cursor auf eine bestimmte Position
// Parameter line: Zeilennummer (0 oder 1)
// Parameter pos: Spaltenposition (0-15)
// 
// Das LCD verwendet DDRAM-Adressen: Zeile 0 beginnt bei 0x00, Zeile 1 bei 0x40
static void lcd_set_cursor(uint8_t line, uint8_t pos) {
    uint8_t addr = (line == 0) ? (0x80 + pos) : (0xC0 + pos);
    lcd_command(addr);
}

// Schreibt eine Textzeile auf das LCD
// Parameter line: Zeilennummer (0 oder 1)
// Parameter text: Null-terminierter String (max. 16 Zeichen)
// 
// Die Zeile wird mit Leerzeichen aufgefüllt, um vorherige Inhalte zu löschen
void lcd_1602_i2c_write_line(uint8_t line, const char* text) {
    if (line > 1) return;  // Ungültige Zeilennummer
    
    lcd_set_cursor(line, 0);
    
    // Puffer für 16 Zeichen + Null-Terminierung
    char buf[17];
    size_t len = strlen(text);
    
    // Begrenzt die Länge auf 16 Zeichen
    if (len > 16) len = 16;
    
    // Kopiert den Text in den Puffer
    memcpy(buf, text, len);
    
    // Füllt den Rest mit Leerzeichen auf
    for (size_t i = len; i < 16; ++i) buf[i] = ' ';
    buf[16] = '\0';
    
    // Sendet alle 16 Zeichen an das Display
    for (size_t i = 0; i < 16; ++i) {
        lcd_char(buf[i]);
    }
}

// Zeigt einen gescannten Barcode auf dem LCD an
// Parameter code: Barcode-String
// 
// Zeile 0: "CODE:"
// Zeile 1: Der Barcode (bis zu 16 Zeichen)
void lcd_1602_i2c_show_barcode(const char* code) {
    lcd_1602_i2c_write_line(0, "CODE:");
    lcd_1602_i2c_write_line(1, code);
}

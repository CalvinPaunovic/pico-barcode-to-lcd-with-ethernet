// Minimaler 16x2 I2C LCD Treiber (PCF8574) – vereinfachte Variante
// Nur die Funktionen, die wir für eine einfache Barcode-Ausgabe brauchen.

#pragma once

#include <stdint.h>

// Initialisiert I2C und das LCD (löscht Display)
void lcd_1602_i2c_init(void);

// Löscht das Display
void lcd_1602_i2c_clear(void);

// Schreibt bis zu 16 Zeichen in eine Zeile (0 oder 1), kürzt falls länger.
void lcd_1602_i2c_write_line(uint8_t line, const char* text);

// Komfort: Zeigt einen Barcode (Zeile 0: "CODE:", Zeile 1: eigentlicher Code, gekürzt)
void lcd_1602_i2c_show_barcode(const char* code);

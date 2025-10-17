// Vereinfachter 16x2 LCD I2C Treiber (PCF8574 Backpack)
// Hinweise: basiert lose auf dem Beispiel lcd_1602_i2c.c, aber stark reduziert.

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
#ifndef LCD_I2C_INSTANCE
#define LCD_I2C_INSTANCE i2c0
#endif

#ifndef LCD_I2C_SDA_PIN
#define LCD_I2C_SDA_PIN PICO_DEFAULT_I2C_SDA_PIN
#endif

#ifndef LCD_I2C_SCL_PIN
#define LCD_I2C_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN
#endif

static int lcd_addr = 0x27; // Standard-Adresse vieler PCF8574 Module

// Kommandos
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME   0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET  0x20

// Flags
#define LCD_ENTRYLEFT    0x02
#define LCD_DISPLAYON    0x04
#define LCD_2LINE        0x08

#define LCD_BACKLIGHT    0x08
#define LCD_ENABLE_BIT   0x04

#define LCD_MODE_CHAR 1
#define LCD_MODE_CMD  0

static void lcd_write_raw(uint8_t val) {
    i2c_write_blocking(LCD_I2C_INSTANCE, lcd_addr, &val, 1, false);
}

static void lcd_toggle(uint8_t val) {
    sleep_us(600);
    lcd_write_raw(val | LCD_ENABLE_BIT);
    sleep_us(600);
    lcd_write_raw(val & ~LCD_ENABLE_BIT);
    sleep_us(600);
}

static void lcd_send(uint8_t value, uint8_t mode) {
    uint8_t high = mode | (value & 0xF0) | LCD_BACKLIGHT;
    uint8_t low  = mode | ((value << 4) & 0xF0) | LCD_BACKLIGHT;
    lcd_write_raw(high); lcd_toggle(high);
    lcd_write_raw(low);  lcd_toggle(low);
}

static void lcd_command(uint8_t cmd) { lcd_send(cmd, LCD_MODE_CMD); }
static void lcd_char(uint8_t c) { lcd_send(c, LCD_MODE_CHAR); }

void lcd_1602_i2c_clear(void) { lcd_command(LCD_CLEARDISPLAY); sleep_ms(2); }

void lcd_1602_i2c_init(void) {
    // I2C Setup (100kHz ausreichend)
    i2c_init(LCD_I2C_INSTANCE, 100 * 1000);
    gpio_set_function(LCD_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_I2C_SDA_PIN);
    gpio_pull_up(LCD_I2C_SCL_PIN);

    // Init Sequenz
    sleep_ms(50);
    lcd_command(0x03); sleep_ms(5);
    lcd_command(0x03); sleep_ms(5);
    lcd_command(0x03); sleep_ms(5);
    lcd_command(0x02); // 4-bit

    lcd_command(LCD_FUNCTIONSET | LCD_2LINE);
    lcd_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
    lcd_1602_i2c_clear();
}

static void lcd_set_cursor(uint8_t line, uint8_t pos) {
    uint8_t addr = (line == 0) ? (0x80 + pos) : (0xC0 + pos);
    lcd_command(addr);
}

void lcd_1602_i2c_write_line(uint8_t line, const char* text) {
    if (line > 1) return;
    lcd_set_cursor(line, 0);
    char buf[17];
    size_t len = strlen(text);
    if (len > 16) len = 16;
    memcpy(buf, text, len);
    for (size_t i = len; i < 16; ++i) buf[i] = ' ';
    buf[16] = '\0';
    for (size_t i = 0; i < 16; ++i) {
        lcd_char(buf[i]);
    }
}

void lcd_1602_i2c_show_barcode(const char* code) {
    lcd_1602_i2c_write_line(0, "CODE:");
    lcd_1602_i2c_write_line(1, code);
}

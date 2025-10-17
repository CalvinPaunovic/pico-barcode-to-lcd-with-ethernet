// Kombiniertes Projekt: USB-HID Barcode -> LCD + Ethernet (CH9121)

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
#define UART_ID0        uart0
#define UART_TX_PIN0    0
#define UART_RX_PIN0    1

// Vorwärtsdeklarationen
void hid_app_task(void);
void led_request_off_ms(uint32_t ms);
void led_service(void);
void cyw43_led_init(void);

// CH9121 Konfiguration (anpassen)
CH9121_Config ch9121_config = {
        .local_ip     = {192, 168, 0, 105},
        .gateway      = {192, 168, 0, 1},
        .subnet_mask  = {255, 255, 255, 0},
        .target_ip    = {192, 168, 0, 86},
        .local_port   = 4000,
        .target_port  = 5000,
        .baud_rate    = 115200,
        .mode         = TCP_CLIENT
};

// LED Deadline Logic
static uint32_t g_led_off_until = 0;
static inline void led_on(void)  { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); }
static inline void led_off(void) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); }
void led_request_off_ms(uint32_t ms) { g_led_off_until = board_millis() + ms; led_off(); }
void led_service(void) { uint32_t now = board_millis(); if ((int32_t)(now - g_led_off_until) < 0) led_off(); else led_on(); }

// Netzwerk-Senden über UART0 -> CH9121
void net_send_line(const char* line) {
        uart_puts(UART_ID0, line);
}

void cyw43_led_init(void) {
    if (cyw43_arch_init()) {
        printf("CYW43 init failed\r\n");
    } else {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
}

int main(void) {
    // Board/TinyUSB
    board_init();
    stdio_init_all();

    printf("USB-HID Barcode -> LCD + Ethernet (CH9121)\r\n");

    cyw43_led_init();
    lcd_1602_i2c_init();

    // TinyUSB Host initialisieren
    tuh_init(BOARD_TUH_RHPORT);
    if (board_init_after_tusb) { board_init_after_tusb(); }

    // CH9121 konfigurieren
    CH9121_configure(&ch9121_config);

    // UART0 für Datenverkehr mit CH9121
    uart_init(UART_ID0, ch9121_config.baud_rate);
    gpio_set_function(UART_TX_PIN0, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN0, GPIO_FUNC_UART);

    // kurze Wartezeit
    sleep_ms(500);

    while (1) {
        // TinyUSB Host Polling
        tuh_task();
        // HID App (Barcode Verarbeitung)
        hid_app_task();
        // LED Service
        led_service();
    }
}

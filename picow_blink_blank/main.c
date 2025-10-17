/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

int main() {
    stdio_init_all();
    
    // Warten bis USB Serial bereit ist
    sleep_ms(2000);
    
    printf("Hello, world!\n");
    printf("Pico W Blink-Programm gestartet!\n");
    
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }
    
    printf("Wi-Fi Chip initialisiert erfolgreich!\n");
    printf("LED Blink-Schleife startet...\n");
    
    int counter = 0;
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        printf("LED AN - Count: %d\n", counter);
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        printf("LED AUS\n");
        sleep_ms(250);
        counter++;
    }
}
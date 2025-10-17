/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "pico/cyw43_arch.h"
#include "lcd_1602_i2c.h"


// **************************************************************************************************************** //

// Ursprung vor example projects der Raspberry Pi Pico Extension
// Die Projekte host_cdc_msc_hid, lcd_1602_i2c und picow_blink (alle für den Pico 2W) wurden in diesem Projekt fusioniert.

// **************************************************************************************************************** //


//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
void led_blinking_task(void);
void cyw43_led_init(void);
void led_on(void);
void led_off(void);
void led_request_off_ms(uint32_t ms);
void led_service(void);
extern void cdc_app_task(void);
extern void hid_app_task(void);

#if CFG_TUH_ENABLED && CFG_TUH_MAX3421
// API to read/rite MAX3421's register. Implemented by TinyUSB
extern uint8_t tuh_max3421_reg_read(uint8_t rhport, uint8_t reg, bool in_isr);
extern bool tuh_max3421_reg_write(uint8_t rhport, uint8_t reg, uint8_t data, bool in_isr);
#endif

/*------------- MAIN -------------*/
int main(void) {
  board_init();

  printf("TinyUSB Host CDC MSC HID Example\r\n");

  cyw43_led_init();
  lcd_1602_i2c_init();

  // init host stack on configured roothub port
  tuh_init(BOARD_TUH_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  #if CFG_TUH_ENABLED && CFG_TUH_MAX3421
    // FeatherWing MAX3421E use MAX3421E's GPIO0 for VBUS enable
    enum { IOPINS1_ADDR  = 20u << 3, /* 0xA0 */ };
    tuh_max3421_reg_write(BOARD_TUH_RHPORT, IOPINS1_ADDR, 0x01, false);
  #endif


  while (1) {
    tuh_task();


    // led_blinking_task();

    led_service();


    // cdc_app_task();
    hid_app_task();
  }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+
void tuh_mount_cb(uint8_t dev_addr) {
  // application set-up
  printf("A device with address %d is mounted\r\n", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr) {
  // application tear-down
  printf("A device with address %d is unmounted \r\n", dev_addr);
}


//--------------------------------------------------------------------+
// LED initialization (Pico 2W spezifisch)
//--------------------------------------------------------------------+
void cyw43_led_init(void) {
  if (cyw43_arch_init()) {
    printf("CYW43 init failed\r\n");
    // Falls die Initialisierung fehlschlägt, weiterlaufen ohne WiFi-LED
  } else {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // LED aus
  }
}

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  const uint32_t interval_ms = 500;
  static uint32_t last_ms = 0;  // Merkt sich, wann die LED zuletzt getoggelt wurde
  static bool led_state = false;  // Merkt sich, ob LED an oder aus ist

  if (board_millis() - last_ms < interval_ms) return;  // Wenn noch nicht keine 500ms vergangen sind, nichts tun
  last_ms += interval_ms;  // Wenn mehr als 500ms vergangen sind, LED toggeln

  // board_led_write(led_state);  // LED toggeln (allgemein)
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state ? 1 : 0);  // LED toggeln (Pico 2W spezifisch)
  led_state = !led_state;
}

void led_on(void) {
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

void led_off(void) {
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}


//--------------------------------------------------------------------+
// LED Deadline Logic
//--------------------------------------------------------------------+
// Prüft, ob LED aus oder an sein soll
void led_service(void) {
  extern uint32_t g_led_off_until;
  uint32_t now = board_millis();
  if ((int32_t)(now - g_led_off_until) < 0) {
    led_off();
  } else {
    led_on();
  }
}
uint32_t g_led_off_until = 0;
// Schaltet LED für die übergebene Zeit (ms) aus. Wird von hid_app.c aufgerufen, wenn eine Keyboard-Eingabe erfolgt.
void led_request_off_ms(uint32_t ms) {
  // aktueller Zeitpunkt + übergebene Zeit = g_led_off_until. 
  // z.B. 1000 + 300 = 1300. Also: LED soll bis 1300 ms aus bleiben
  g_led_off_until = board_millis() + ms;
  led_off();
}
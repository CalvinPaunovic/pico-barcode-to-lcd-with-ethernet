/*
 * HID handler adapted to forward scanned barcodes to LCD and Ethernet (CH9121)
 */

#include "bsp/board_api.h"
#include "tusb.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "lcd_1602_i2c.h"

// Methoden aus main.c
void led_request_off_ms(uint32_t ms);
void net_send_line(const char* line);

#define MAX_REPORT  4

static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };

static struct {
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

void hid_app_task(void) {
  // nothing to do
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  if ( itf_protocol == HID_ITF_PROTOCOL_NONE ) {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    printf("HID has %u reports \r\n", hid_info[instance].report_count);
  }

  if ( !tuh_hid_receive_report(dev_addr, instance) ) {
    printf("Error: cannot request to receive report\r\n");
  }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol) {
    case HID_ITF_PROTOCOL_KEYBOARD:
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;
    case HID_ITF_PROTOCOL_MOUSE:
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;
    default:
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  if ( !tuh_hid_receive_report(dev_addr, instance) ) {
    printf("Error: cannot request to receive report\r\n");
  }
}

static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode) {
  for(uint8_t i=0; i<6; i++) {
    if (report->keycode[i] == keycode)  return true;
  }
  return false;
}

static void process_kbd_report(hid_keyboard_report_t const *report) {
  static hid_keyboard_report_t prev_report = { 0, 0, {0} };
  static char barcode_buf[64];
  static size_t barcode_len = 0;

  for(uint8_t i=0; i<6; i++) {
    if ( report->keycode[i] ) {
      if (!find_key_in_report(&prev_report, report->keycode[i])) {
        bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
        uint8_t ch = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];

        if ( ch == '\r' ) {
          // finalize barcode
          if (barcode_len > 0) {
            barcode_buf[barcode_len] = '\0';
            lcd_1602_i2c_show_barcode(barcode_buf);
            // Send to Ethernet (UART0 -> CH9121) with newline
            char line[80];
            snprintf(line, sizeof(line), "%s\n", barcode_buf);
            net_send_line(line);
            // LED feedback
            led_request_off_ms(3000);
            barcode_len = 0;
          }
        } else if (ch >= 32 && ch <= 126) {
          if (barcode_len < sizeof(barcode_buf)-1) {
            barcode_buf[barcode_len++] = (char)ch;
          }
          led_request_off_ms(300);
        } else {
          // ignore non-printable
        }
      }
    }
  }
  prev_report = *report;
}

static void process_mouse_report(hid_mouse_report_t const * report) {
  (void)report;
}

static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  (void) dev_addr; (void) len;
  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
    rpt_info = &rpt_info_arr[0];
  } else {
    uint8_t const rpt_id = report[0];
    for(uint8_t i=0; i<rpt_count; i++) {
      if (rpt_id == rpt_info_arr[i].report_id ) { rpt_info = &rpt_info_arr[i]; break; }
    }
    report++; len--;
  }

  if (!rpt_info) return;

  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP ) {
    switch (rpt_info->usage) {
      case HID_USAGE_DESKTOP_KEYBOARD:
        process_kbd_report( (hid_keyboard_report_t const*) report );
      break;
      default: break;
    }
  }
}

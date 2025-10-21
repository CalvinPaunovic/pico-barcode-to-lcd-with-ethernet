/*
 * HID handler adapted to forward scanned barcodes to LCD and Ethernet (CH9121)
 * 
 * Diese Datei implementiert die HID-Verarbeitungslogik für USB-Eingabegeräte,
 * insbesondere für Barcode-Scanner, die als USB-HID-Tastatur fungieren.
 * Gescannte Barcodes werden auf dem LCD angezeigt und über Ethernet versendet.
 */

#include "bsp/board_api.h"
#include "tusb.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "lcd_1602_i2c.h"

// Methoden aus main.c
// Externe Funktionen für LED-Steuerung und Netzwerkkommunikation
void led_request_off_ms(uint32_t ms);
void net_send_line(const char* line);

// Maximale Anzahl von Reports pro HID-Gerät
#define MAX_REPORT  4

// Lookup-Tabelle für die Konvertierung von HID-Keycodes in ASCII-Zeichen
// Index: Keycode, [0]: normale Taste, [1]: mit Shift-Taste
static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };

// Struktur zur Speicherung von HID-Report-Informationen pro Gerät
static struct {
  uint8_t report_count;                             // Anzahl der Reports
  tuh_hid_report_info_t report_info[MAX_REPORT];    // Report-Informationen
} hid_info[CFG_TUH_HID];

// Funktionsprototypen für Report-Verarbeitung
static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

// HID-App-Task (aktuell keine Aktion erforderlich)
// Diese Funktion wird in der Hauptschleife aufgerufen, aktuell werden alle Aktionen
// durch Callbacks ausgeführt
void hid_app_task(void) {
  // nothing to do
}

// Callback-Funktion, die aufgerufen wird, wenn ein HID-Gerät angeschlossen wird
// Parameter dev_addr: USB-Geräteadresse
// Parameter instance: Instanznummer des HID-Interface
// Parameter desc_report: Zeiger auf den Report-Deskriptor
// Parameter desc_len: Länge des Report-Deskriptors
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

  // Protokoll-Strings für Debug-Ausgabe
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  // Wenn kein spezifisches Protokoll erkannt wurde, Report-Deskriptor parsen
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE ) {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    printf("HID has %u reports \r\n", hid_info[instance].report_count);
  }

  // Fordert den ersten Report vom Gerät an
  if ( !tuh_hid_receive_report(dev_addr, instance) ) {
    printf("Error: cannot request to receive report\r\n");
  }
}

// Callback-Funktion, die aufgerufen wird, wenn ein HID-Gerät getrennt wird
// Parameter dev_addr: USB-Geräteadresse
// Parameter instance: Instanznummer des HID-Interface
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Callback-Funktion, die aufgerufen wird, wenn ein HID-Report empfangen wurde
// Parameter dev_addr: USB-Geräteadresse
// Parameter instance: Instanznummer des HID-Interface
// Parameter report: Zeiger auf die Report-Daten
// Parameter len: Länge der Report-Daten
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  // Ermittelt das Interface-Protokoll (Tastatur, Maus oder generisch)
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  // Verarbeitet den Report basierend auf dem Gerätetyp
  switch (itf_protocol) {
    case HID_ITF_PROTOCOL_KEYBOARD:
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;
    case HID_ITF_PROTOCOL_MOUSE:
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;
    default:
      // Für generische HID-Geräte (z.B. Barcode-Scanner ohne Tastatur-Protokoll)
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // Fordert den nächsten Report an
  if ( !tuh_hid_receive_report(dev_addr, instance) ) {
    printf("Error: cannot request to receive report\r\n");
  }
}

// Hilfsfunktion zur Überprüfung, ob ein bestimmter Keycode im Report vorhanden ist
// Parameter report: Zeiger auf den Tastatur-Report
// Parameter keycode: Zu suchender Keycode
// Rückgabe: true, wenn der Keycode gefunden wurde, sonst false
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode) {
  for(uint8_t i=0; i<6; i++) {
    if (report->keycode[i] == keycode)  return true;
  }
  return false;
}

// Verarbeitet Tastatur-Reports und sammelt Zeichen zu einem Barcode
// Parameter report: Zeiger auf den Tastatur-Report
// 
// Diese Funktion akkumuliert eingehende Zeichen in einem Puffer, bis die Enter-Taste
// gedrückt wird. Dann wird der komplette Barcode auf dem LCD angezeigt und über
// Ethernet versendet.
static void process_kbd_report(hid_keyboard_report_t const *report) {
  // Statische Variablen für die Verfolgung des vorherigen Reports und des Barcode-Puffers
  static hid_keyboard_report_t prev_report = { 0, 0, {0} };  // Letzter empfangener Report
  static char barcode_buf[64];                                // Puffer für Barcode-Zeichen
  static size_t barcode_len = 0;                              // Aktuelle Länge des Barcodes

  // Iteriert über alle Keycodes im aktuellen Report
  for(uint8_t i=0; i<6; i++) {
    if ( report->keycode[i] ) {
      // Verarbeitet nur neu gedrückte Tasten (nicht im vorherigen Report)
      if (!find_key_in_report(&prev_report, report->keycode[i])) {
        // Überprüft, ob eine Shift-Taste gedrückt ist
        bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
        
        // Konvertiert den Keycode in ein ASCII-Zeichen
        uint8_t ch = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];

        if ( ch == '\r' ) {
          // finalize barcode
          // Enter-Taste erkannt: Barcode ist komplett
          if (barcode_len > 0) {
            barcode_buf[barcode_len] = '\0';  // Null-Terminierung
            
            // Zeigt den Barcode auf dem LCD an
            lcd_1602_i2c_show_barcode(barcode_buf);
            
            // Send to Ethernet (UART0 -> CH9121) with newline
            // Sendet den Barcode über Ethernet mit Zeilenumbruch
            char line[80];
            snprintf(line, sizeof(line), "%s\n", barcode_buf);
            net_send_line(line);
            
            // LED feedback
            // LED-Feedback: LED für 3 Sekunden ausschalten
            led_request_off_ms(3000);
            
            // Setzt den Barcode-Puffer zurück
            barcode_len = 0;
          }
        } else if (ch >= 32 && ch <= 126) {
          // Druckbare ASCII-Zeichen (Space bis Tilde)
          // Fügt das Zeichen zum Barcode-Puffer hinzu, wenn noch Platz ist
          if (barcode_len < sizeof(barcode_buf)-1) {
            barcode_buf[barcode_len++] = (char)ch;
          }
          // Kurzes LED-Feedback für jedes empfangene Zeichen
          led_request_off_ms(300);
        } else {
          // ignore non-printable
          // Ignoriert nicht druckbare Zeichen
        }
      }
    }
  }
  // Speichert den aktuellen Report für den nächsten Vergleich
  prev_report = *report;
}

// Verarbeitet Maus-Reports (aktuell keine Aktion)
// Parameter report: Zeiger auf den Maus-Report
static void process_mouse_report(hid_mouse_report_t const * report) {
  (void)report;  // Ungenutzt, verhindert Compiler-Warnung
}

// Verarbeitet generische HID-Reports
// Parameter dev_addr: USB-Geräteadresse
// Parameter instance: Instanznummer des HID-Interface
// Parameter report: Zeiger auf die Report-Daten
// Parameter len: Länge der Report-Daten
// 
// Diese Funktion wird für HID-Geräte aufgerufen, die kein Standard-Tastatur- oder
// Maus-Protokoll verwenden. Prüft, ob das Gerät ein Tastatur-Usage hat und
// verarbeitet es entsprechend.
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
  (void) dev_addr; (void) len;  // Ungenutzte Parameter
  
  // Ruft die Report-Informationen für diese Instanz ab
  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  // Ermittelt die passende Report-Info
  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
    // Nur ein Report und keine Report-ID
    rpt_info = &rpt_info_arr[0];
  } else {
    // Sucht nach der Report-ID im ersten Byte
    uint8_t const rpt_id = report[0];
    for(uint8_t i=0; i<rpt_count; i++) {
      if (rpt_id == rpt_info_arr[i].report_id ) { rpt_info = &rpt_info_arr[i]; break; }
    }
    report++; len--;  // Überspringt die Report-ID
  }

  if (!rpt_info) return;  // Keine passende Report-Info gefunden

  // Überprüft, ob das Gerät als Desktop-Device (z.B. Tastatur) klassifiziert ist
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP ) {
    switch (rpt_info->usage) {
      case HID_USAGE_DESKTOP_KEYBOARD:
        // Verarbeitet den Report als Tastatur-Report
        process_kbd_report( (hid_keyboard_report_t const*) report );
      break;
      default: break;
    }
  }
}

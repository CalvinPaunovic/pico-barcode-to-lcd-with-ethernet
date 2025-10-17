/*****************************************************************************
 * CH9121 IP Configuration & Communication Tool
 * 
 * This program:
 * 1. Configures CH9121 Ethernet module (settings saved to EEPROM)
 * 2. Sends continuous status messages to console and network partners
 * 
 * HOW TO USE:
 * 1. Edit the IP configuration values below
 * 2. Compile and flash to Pico
 * 3. Run once - CH9121 is configured and starts sending data
 * 
 *****************************************************************************/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "CH9121.h"

// UART1 is used for data communication with CH9121 (network traffic)
#define UART_ID0        uart0
#define UART_TX_PIN0    0
#define UART_RX_PIN0    1
#define DATA_BAUD_RATE  115200  // Must match CH9121 baud rate config


// **************************************************************************************************************** //

// Verbindet sich als TCP-Client mit einem Python-Zwischenserver (IP-Adresse des PCs)
// Der Python-Server kommuniziert dann mit der MySQL-Datenbank

// **************************************************************************************************************** //


// ============================================================================
// CH9121 CONFIGURATION - CHANGE THESE VALUES AS NEEDED
// ============================================================================

/*
 0: TCP-Server, der CH9121 wartet auf eingehende Verbindung auf dem localport. Empfängt Daten.
 Beispiel: telnet [eth.localip] [eth.u0localport]

 1: TCP-Client, der CH9121 verbindet sich mit dem angegebenen Server. Versendet Daten.
 Beispiel: mit tcp_server.py

 2: UDP-Server, der CH9121 empfängt UDP-Pakete über localport.

 3: UDP-Client, der CH9121 versendet einfach ohne Verbindungsaufbau Daten an einen angegebenen Port.
*/

// Für Python-Zwischenserver (der dann mit MySQL kommuniziert)
CH9121_Config ch9121_config = {
    .local_ip     = {192, 168, 0, 105},   // Local IP of CH9121
    .gateway      = {192, 168, 0, 1},     // Gateway
    .subnet_mask  = {255, 255, 255, 0},   // Subnet Mask
    .target_ip    = {192, 168, 0, 86},    // Python-Server IP (IP-Adresse des PCs)
    .local_port   = 4000,                 // Local Port
    .target_port  = 5000,                 // Python-Server Port
    .baud_rate    = 115200,               // UART Baud Rate
    .mode         = TCP_CLIENT            // TCP Client
};

// ============================================================================
// MAIN PROGRAM
// ============================================================================
int main() {
    // Initialize stdio (for console debug output via USB)
    stdio_init_all();
    
    // Configure CH9121 with settings above
    CH9121_configure(&ch9121_config);
    
    // Initialize UART0 for data communication with CH9121 (network traffic)
    uart_init(UART_ID0, ch9121_config.baud_rate);
    gpio_set_function(UART_TX_PIN0, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN0, GPIO_FUNC_UART);
    
    printf("\n==============================================\n");
    printf("Connecting to Python MySQL Bridge...\n");
    printf("Server: %d.%d.%d.%d:%d\n", 
           ch9121_config.target_ip[0], ch9121_config.target_ip[1],
           ch9121_config.target_ip[2], ch9121_config.target_ip[3],
           ch9121_config.target_port);
    printf("==============================================\n\n");
    
    // Counter for message numbering
    uint32_t message_count = 0;
    
    // Warte kurz auf Verbindungsaufbau
    sleep_ms(2000);
    
    // Main loop: Send continuous status updates
    while (true) {
        uint32_t time_ms = to_ms_since_boot(get_absolute_time());
        
        // Simulierter Barcode (in echter Anwendung von Scanner lesen)
        char barcode[32];
        snprintf(barcode, sizeof(barcode), "BAR%08lu", message_count);
        
        // Einfaches Text-Protokoll: nur der Barcode mit Newline
        char message[64];
        snprintf(message, sizeof(message), "%s\n", barcode);
        
        // Debug-Ausgabe auf Console
        printf("[%lu] Sending barcode: %s", time_ms, message);
        
        // Send to Python server via UART0 -> CH9121 -> Python
        uart_puts(UART_ID0, message);
        
        message_count++;
        sleep_ms(1000); // Sende alle 1 Sekunde einen neuen Barcode
    }
    
    return 0;
}

#ifndef _CH9121_H_
#define _CH9121_H_

#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define UART_ID0        uart0
#define UART_TX_PIN0    0
#define UART_RX_PIN0    1
#define CFG_PIN         14      // Configuration pin for CH9121
#define RES_PIN         17      // Reset pin for CH9121
#define BAUD_RATE       9600    // CH9121 config mode uses 9600 baud

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================
#define UCHAR           unsigned char
#define UBYTE           uint8_t
#define UWORD           uint16_t
#define UDOUBLE         uint32_t

// ============================================================================
// CH9121 MODES
// ============================================================================
#define TCP_SERVER      0       // TCP Server mode
#define TCP_CLIENT      1       // TCP Client mode
#define UDP_SERVER      2       // UDP Server mode
#define UDP_CLIENT      3       // UDP Client mode

// ============================================================================
// CH9121 COMMAND CODES
// ============================================================================
#define CMD_MODE            0x10    // Set mode
#define CMD_LOCAL_IP        0x11    // Set local IP
#define CMD_SUBNET_MASK     0x12    // Set subnet mask
#define CMD_GATEWAY         0x13    // Set gateway
#define CMD_LOCAL_PORT1     0x14    // Set local port
#define CMD_TARGET_IP1      0x15    // Set target IP
#define CMD_TARGET_PORT1    0x16    // Set target port
#define CMD_UART1_BAUD1     0x21    // Set UART baud rate

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================
typedef struct {
    UCHAR local_ip[4];          // Local IP address
    UCHAR gateway[4];           // Gateway address
    UCHAR subnet_mask[4];       // Subnet mask
    UCHAR target_ip[4];         // Target IP (for TCP/UDP Client mode)
    UWORD local_port;           // Local port
    UWORD target_port;          // Target port
    UDOUBLE baud_rate;          // UART baud rate
    UCHAR mode;                 // Mode: 0=TCP Server, 1=TCP Client, 2=UDP Server, 3=UDP Client
} CH9121_Config;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * Configure CH9121 module with network settings
 * Settings are permanently saved to CH9121's internal EEPROM
 * 
 * @param config Pointer to CH9121_Config structure with all parameters
 */
void CH9121_configure(CH9121_Config *config);

/**
 * Delay functions
 */
void DEV_Delay_ms(UDOUBLE xms);
void DEV_Delay_us(UDOUBLE xus);

#endif

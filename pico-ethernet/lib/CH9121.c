#include "CH9121.h"
#include <stdio.h>

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Send configuration command with 1 data byte
static void send_ch9121_command_1byte(UCHAR cmd, UCHAR data) {
    UCHAR packet[4] = {0x57, 0xAB, cmd, data};
    for (int i = 0; i < 4; i++) {
        uart_putc(UART_ID0, packet[i]);
    }
    DEV_Delay_ms(10);
}

// Send configuration command with 2 data bytes (for ports)
static void send_ch9121_command_2bytes(UCHAR cmd, UWORD data) {
    UCHAR packet[5] = {0x57, 0xAB, cmd, (data & 0xFF), (data >> 8)};
    for (int i = 0; i < 5; i++) {
        uart_putc(UART_ID0, packet[i]);
    }
    DEV_Delay_ms(10);
}
// Send configuration command with 4 data bytes (for IP addresses)
static void send_ch9121_command_4bytes(UCHAR cmd, UCHAR data[4]) {
    UCHAR packet[7] = {0x57, 0xAB, cmd, data[0], data[1], data[2], data[3]};
    for (int i = 0; i < 7; i++) {
        uart_putc(UART_ID0, packet[i]);
    }
    DEV_Delay_ms(10);
}

// Send configuration command with 4 data bytes (for baud rate)
static void send_ch9121_baud_rate(UCHAR cmd, UDOUBLE baud) {
    UCHAR packet[7] = {
        0x57, 0xAB, cmd,
        (baud & 0xFF),
        (baud >> 8) & 0xFF,
        (baud >> 16) & 0xFF,
        (baud >> 24) & 0xFF
    };
    for (int i = 0; i < 7; i++) {
        uart_putc(UART_ID0, packet[i]);
    }
    DEV_Delay_ms(10);
}

// Save configuration to CH9121 EEPROM (makes settings permanent)
static void save_ch9121_config() {
    UCHAR packet[3] = {0x57, 0xAB, 0x0D};
    
    // Command 1: Prepare to save
    packet[2] = 0x0D;
    for (int i = 0; i < 3; i++) uart_putc(UART_ID0, packet[i]);
    DEV_Delay_ms(200);
    
    // Command 2: Confirm save
    packet[2] = 0x0E;
    for (int i = 0; i < 3; i++) uart_putc(UART_ID0, packet[i]);
    DEV_Delay_ms(200);
    
    // Command 3: Execute save
    packet[2] = 0x5E;
    for (int i = 0; i < 3; i++) uart_putc(UART_ID0, packet[i]);
    DEV_Delay_ms(200);
}

/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
    sleep_ms(xms);
}

void DEV_Delay_us(UDOUBLE xus)
{
    sleep_us(xus);
}


/******************************************************************************
function:	CH9121_configure
parameter:
    config: CH9121_Config structure with all configuration parameters
Info:  Configure CH9121 and save settings permanently to EEPROM
******************************************************************************/
void CH9121_configure(CH9121_Config *config)
{
    // Initialize UART0 for communication with CH9121
    uart_init(UART_ID0, BAUD_RATE);  // CH9121 config mode uses 9600 baud
    gpio_set_function(UART_TX_PIN0, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN0, GPIO_FUNC_UART);
    
    // Initialize control pins
    gpio_init(CFG_PIN);
    gpio_init(RES_PIN);
    gpio_set_dir(CFG_PIN, GPIO_OUT);
    gpio_set_dir(RES_PIN, GPIO_OUT);
    
    // Keep CH9121 in normal operation
    gpio_put(RES_PIN, 1);
    
    // Enter configuration mode (CFG_PIN = LOW)
    printf("Entering CH9121 configuration mode...\n");
    gpio_put(CFG_PIN, 0);
    DEV_Delay_ms(500);
    
    // Send all configuration parameters
    printf("Configuring CH9121...\n");
    
    DEV_Delay_ms(100);
    send_ch9121_command_1byte(CMD_MODE, config->mode);
    printf("  - Mode: %s\n", config->mode == 1 ? "TCP Client" : "Other");
    
    DEV_Delay_ms(100);
    send_ch9121_command_4bytes(CMD_LOCAL_IP, config->local_ip);
    printf("  - Local IP: %d.%d.%d.%d\n", 
           config->local_ip[0], config->local_ip[1], 
           config->local_ip[2], config->local_ip[3]);
    
    DEV_Delay_ms(100);
    send_ch9121_command_4bytes(CMD_SUBNET_MASK, config->subnet_mask);
    printf("  - Subnet Mask: %d.%d.%d.%d\n",
           config->subnet_mask[0], config->subnet_mask[1],
           config->subnet_mask[2], config->subnet_mask[3]);
    
    DEV_Delay_ms(100);
    send_ch9121_command_4bytes(CMD_GATEWAY, config->gateway);
    printf("  - Gateway: %d.%d.%d.%d\n",
           config->gateway[0], config->gateway[1],
           config->gateway[2], config->gateway[3]);
    
    DEV_Delay_ms(100);
    send_ch9121_command_4bytes(CMD_TARGET_IP1, config->target_ip);
    printf("  - Target IP: %d.%d.%d.%d\n",
           config->target_ip[0], config->target_ip[1],
           config->target_ip[2], config->target_ip[3]);
    
    DEV_Delay_ms(100);
    send_ch9121_command_2bytes(CMD_LOCAL_PORT1, config->local_port);
    printf("  - Local Port: %d\n", config->local_port);
    
    DEV_Delay_ms(100);
    send_ch9121_command_2bytes(CMD_TARGET_PORT1, config->target_port);
    printf("  - Target Port: %d\n", config->target_port);
    
    DEV_Delay_ms(100);
    send_ch9121_baud_rate(CMD_UART1_BAUD1, config->baud_rate);
    printf("  - Baud Rate: %d\n", config->baud_rate);
    
    // Save configuration to EEPROM (permanent storage)
    DEV_Delay_ms(100);
    printf("\nSaving configuration to CH9121 EEPROM...\n");
    save_ch9121_config();
    
    // Exit configuration mode (CFG_PIN = HIGH)
    DEV_Delay_ms(100);
    gpio_put(CFG_PIN, 1);
    
    printf("\n==============================================\n");
    printf("CH9121 configuration complete!\n");
    printf("Settings are permanently saved to EEPROM.\n");
    printf("==============================================\n");
}

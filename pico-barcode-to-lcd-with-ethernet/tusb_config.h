/*
 * TinyUSB Host Configuration for Pico 2W
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

// Common Configuration
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

#ifndef CFG_TUH_MEM_SECTION
#define CFG_TUH_MEM_SECTION
#endif

#ifndef CFG_TUH_MEM_ALIGN
#define CFG_TUH_MEM_ALIGN     __attribute__ ((aligned(4)))
#endif

// Host Configuration
#define CFG_TUH_ENABLED       1

#if CFG_TUSB_MCU == OPT_MCU_RP2040
  // Using on-chip USB host controller
  // #define CFG_TUH_RPI_PIO_USB   1 // alternative PIO USB
  // #define CFG_TUH_MAX3421       1 // external MAX3421
  #if (defined(CFG_TUH_RPI_PIO_USB) && CFG_TUH_RPI_PIO_USB) || (defined(CFG_TUH_MAX3421) && CFG_TUH_MAX3421)
    #define BOARD_TUH_RHPORT      1
  #endif
#endif

#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT      0
#endif

#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

#define CFG_TUH_MAX_SPEED     BOARD_TUH_MAX_SPEED

// Driver Configuration
#define CFG_TUH_ENUMERATION_BUFSIZE 256

#define CFG_TUH_HUB                 1
#define CFG_TUH_CDC                 1
#define CFG_TUH_CDC_FTDI            1
#define CFG_TUH_CDC_CP210X          1
#define CFG_TUH_CDC_CH34X           1
#define CFG_TUH_HID                 (3*CFG_TUH_DEVICE_MAX)
#define CFG_TUH_MSC                 1
#define CFG_TUH_VENDOR              0

#define CFG_TUH_DEVICE_MAX          (3*CFG_TUH_HUB + 1)

#define CFG_TUH_HID_EPIN_BUFSIZE    64
#define CFG_TUH_HID_EPOUT_BUFSIZE   64

#define CFG_TUH_CDC_LINE_CONTROL_ON_ENUM    0x03
#define CFG_TUH_CDC_LINE_CODING_ON_ENUM   { 115200, CDC_LINE_CODING_STOP_BITS_1, CDC_LINE_CODING_PARITY_NONE, 8 }

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */

# Generated Cmake Pico project file
# 
# Diese CMakeLists.txt-Datei konfiguriert das Build-System für das Barcode-LCD-Ethernet-Projekt
# Definiert Abhängigkeiten, Bibliotheken und Build-Optionen

# Mindestversion von CMake, die für dieses Projekt erforderlich ist
cmake_minimum_required(VERSION 3.13)

# Setzt den C-Standard auf C11
set(CMAKE_C_STANDARD 11)

# Setzt den C++-Standard auf C++17
set(CMAKE_CXX_STANDARD 17)

# Exportiert Compile-Befehle für IDE-Integration (z.B. IntelliSense)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
# Automatische SDK-Konfiguration durch die VS Code Extension
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
# SDK-Version und Toolchain-Version, die von der Extension verwaltet werden
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
# Inkludiert die VS Code Extension-spezifische CMake-Konfiguration
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Definiert das Ziel-Board: Raspberry Pi Pico 2 W (mit WLAN)
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
# Inkludiert das Pico SDK (muss vor der Projektdefinition erfolgen)
include(pico_sdk_import.cmake)

# Definiert das Projekt mit Name und unterstützten Sprachen
project(main C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
# Initialisiert das Pico SDK (lädt alle Bibliotheken und Toolchain-Definitionen)
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

# Erstellt die ausführbare Datei aus den Quelldateien
add_executable(main
    main.c          # Hauptprogramm mit Initialisierung und Hauptschleife
    hid_app.c       # HID-Verarbeitung (Barcode-Scanner)
    lcd_1602_i2c.c  # LCD-Display-Treiber
    )

# Library for CH9121 driver
# Erstellt eine separate Bibliothek für den CH9121 Ethernet-Treiber
add_library(ch9121
    lib/CH9121.c    # CH9121 Treiber-Implementierung
)

# Fügt Include-Verzeichnisse für die CH9121-Bibliothek hinzu
target_include_directories(ch9121 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Verknüpft die CH9121-Bibliothek mit Pico-Standardbibliotheken
target_link_libraries(ch9121 PUBLIC
    pico_stdlib     # Pico-Standardfunktionen (GPIO, Timer, etc.)
    hardware_uart   # UART-Hardware-Treiber
)

# Make sure TinyUSB can find tusb_config.h
# Fügt das aktuelle Verzeichnis zu den Include-Pfaden hinzu (für tusb_config.h)
target_include_directories(main PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Verknüpft das Hauptprogramm mit allen benötigten Bibliotheken
target_link_libraries(main
    ch9121                  # CH9121 driver library - Ethernet-Modul-Treiber
    pico_stdlib             # for core functionality - Kern-Funktionalität
    hardware_uart           # for UART communication with CH9121 - UART für CH9121
    pico_cyw43_arch_none    # board support - Board-Support für CYW43 (WLAN-Chip)
    tinyusb_host            # TinyUSB Host stack - USB-Host-Funktionalität
    tinyusb_board           # TinyUSB Board support - Board-spezifischer USB-Support
    hardware_i2c            # for LCD I2C - I2C-Hardware für LCD-Kommunikation
)

# Enable USB output, disable UART output
# As USB is used as Host, disable device stdio over USB; also disable UART stdio to keep UART0 free for CH9121
# Deaktiviert USB-Stdio, da USB im Host-Modus verwendet wird
pico_enable_stdio_usb(main 0)

# Deaktiviert UART-Stdio, um UART0 für CH9121-Kommunikation freizuhalten
pico_enable_stdio_uart(main 0)

# create map/bin/hex file etc.
# Erzeugt zusätzliche Output-Dateien (UF2, BIN, HEX, Map-Datei)
pico_add_extra_outputs(main)
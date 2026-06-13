// Meshnology W10 — ESP32-S3R8, E22-900MM22S (SX1262), L76KB-A58 GPS,
// SSD1306-style OLED (SPI), AXP2101 PMU, TCA9555 I/O expander, PCF85063A RTC,
// QMI8658C IMU, SHT41 TH sensor, ES8311 audio codec.
//
// Schematic reference: W10-MB-Sch-V1_1-0501106.pdf (4 pages, 2025-11-06)
// V1.1 revision note (page 2): OLED CLK moved IO7→IO12, MOSI moved IO8→IO13;
// GPIO7/GPIO8 reassigned to I2C (ESP_SCL/ESP_SDA).
//
// IO expander: TCA9555PWR (U7) on ESP_SCL/ESP_SDA (GPIO7/GPIO8).
// EXIO0-EXIO7  = TCA9555 port-0 bits P00–P07
// EXIO8-EXIO15 = TCA9555 port-1 bits P10–P17
// I2C address:  A0/A1/A2 pins — exact pull levels need board inspection;
//               default (all low) → 0x20.  IO_EXPANDER define below must match.
//
// Readyline device: https://readyline.meshtastic.com/devices/5bcfeb39-ed68-4770-ad67-7261d34f709b

#pragma once

// ---------------------------------------------------------------------------
// Board identity
// ---------------------------------------------------------------------------
#define MESHNOLOGY_W10 1

// ---------------------------------------------------------------------------
// I/O expander (TCA9555 at default addr 0x20 — verify A0/A1/A2 pull levels)
// ---------------------------------------------------------------------------
// IO_EXPANDER is defined via platformio.ini build_flags (-DIO_EXPANDER=0x20).
// Expander pin numbering:  EXIO0=0, EXIO1=1, …, EXIO7=7,
//                          EXIO8=8, EXIO9=9, …, EXIO15=15
// Use (n | IO_EXPANDER) to reference expander pins in defines below.

// ---------------------------------------------------------------------------
// I2C — main bus  (ESP_SCL / ESP_SDA)
// Source: page 1 net labels OLED_CLK→GPIO12 V1.1 note; ESP_SCL=GPIO7, ESP_SDA=GPIO8
// ---------------------------------------------------------------------------
#define I2C_SCL 7 // ESP_SCL — schematic page 1, net ESP_SCL=GPIO7
#define I2C_SDA 8 // ESP_SDA — schematic page 1, net ESP_SDA=GPIO8

// ---------------------------------------------------------------------------
// OLED display — SPI (shared bus with LoRa)
// Source: schematic page 1; V1.1 revision note page 2
// Controller: UNKNOWN — schematic labels block "OLED" only; controller chip and
// resolution not visible in extracted schematic text.  Actual controller must
// be verified on physical hardware before enabling the screen.
//
// Confirmed SPI pin assignments (page 1, V1.1):
//   OLED_CLK  = GPIO12  (shared with LoRa SCK)
//   OLED_MOSI = GPIO13  (shared with LoRa MOSI)
//   OLED_CS   = GPIO10  (chip-select)
//   OLED_DC   = GPIO16  (data/command)
//   OLED_BL   = GPIO6   (backlight enable)
//   OLED_RES  = EXIO1   (reset via TCA9555 P01)
//
// TODO: identify OLED controller (SSD1306/SSD1309/SH1106 etc.) and resolution,
//       then select the correct USE_* define and SSD1306_* / SH1106_* pin names.
//       USE_SPISSD1306 is currently the only SPI OLED path in Screen.cpp, but
//       it hardcodes GEOMETRY_64_48; if the OLED is 128x64, Screen.cpp will need
//       a one-line patch to pass GEOMETRY_128_64, or use a different driver.
// ---------------------------------------------------------------------------
#define HAS_SCREEN 0 // TODO: enable once OLED controller is confirmed on hardware

// ---------------------------------------------------------------------------
// LoRa radio — EBYTE E22-900MM22S (SX1262, 868/915 MHz)
// Source: schematic page 4, U10 E22-900MM22S
// SPI bus shared with OLED (OLED CS and LoRa NSS are separate chip-selects)
// ---------------------------------------------------------------------------
#define USE_SX1262

#define LORA_SCK  12 // E22_SCK  — page 4: shared net with OLED_CLK, GPIO12
#define LORA_MOSI 13 // E22_MOSI — page 4: shared net with OLED_MOSI, GPIO13
#define LORA_MISO 14 // E22_MISO — page 4: E22_MISO→GPIO14
#define LORA_CS   11 // E22_NSS  — page 4: E22_NSS→GPIO11

#define SX126X_CS    LORA_CS
#define SX126X_RESET (3 | IO_EXPANDER) // E22_NRST = EXIO3 → TCA9555 P03; page 4
#define SX126X_BUSY  (10 | IO_EXPANDER) // E22_BUSY = EXIO10 → TCA9555 P12; page 4
#define SX126X_DIO1  (9 | IO_EXPANDER)  // E22_DI01 = EXIO9 → TCA9555 P11; page 4

#define LORA_DIO1 SX126X_DIO1

// RF switch: E22 TXEN tied to DIO2 internally (standard E22 wiring).
// E22_RXEN net present on page 4; DIO2_AS_RF_SWITCH inverts it via SX1262 DIO2 output.
#define SX126X_DIO2_AS_RF_SWITCH

// E22-900MM22S uses TCXO; DIO3_TCXO_VOLTAGE enables it.
// TODO: verify TCXO voltage — E22 spec typically 1.8 V; confirm on hardware.
#define SX126X_DIO3_TCXO_VOLTAGE 1.8

// ---------------------------------------------------------------------------
// GPS — L76KB-A58 (U9), UART0
// Source: schematic page 4; net GPS_TXD→U0RXD, GPS_RXD→U0TXD
// Note: schematic net names are from the GPS module perspective:
//   GPS TXD (module output) → MCU U0RXD (GPIO44)
//   GPS RXD (module input)  → MCU U0TXD (GPIO43)
// ---------------------------------------------------------------------------
#define HAS_GPS 1
#define GPS_RX_PIN 44 // U0RXD — GPS_TXD net; page 4
#define GPS_TX_PIN 43 // U0TXD — GPS_RXD net; page 4

// L76KB-A58 default baud: 9600 bps (NMEA); check module datasheet.
#define GPS_BAUDRATE 9600

// GPS control via expander:
#define GPS_RESET_N (11 | IO_EXPANDER) // RESET_N = EXIO11 → TCA9555 P13; page 4
#define GPS_WAKE_UP (12 | IO_EXPANDER) // WAKE_UP = EXIO12 → TCA9555 P14; page 4

// 1PPS: connected to GPS module pin 4 (1PPS net) — GPIO destination not readable
// from schematic text extraction.
// TODO: verify 1PPS GPIO — trace 1PPS net to MCU pin on physical schematic.

// ---------------------------------------------------------------------------
// Power management — AXP2101 (U3) on I2C main bus
// Source: schematic page 3
// ---------------------------------------------------------------------------
#define HAS_AXP2101 1
#define PMU_IRQ (5 | IO_EXPANDER) // AXP_IRQ = EXIO5 → TCA9555 P05; page 3

// AXP2101 does not expose an ADC pin to the MCU; battery voltage read via I2C.
// No BATTERY_PIN / ADC_MULTIPLIER needed.

// ---------------------------------------------------------------------------
// RTC — PCF85063ATL (U8) on I2C main bus (addr 0x51)
// Source: schematic page 3; U8=PCF85063ATL; RTC_INT=EXIO4
// Note: PCF85063 (not PCF8563) — requires lewisxhe/SensorLib in lib_deps
// ---------------------------------------------------------------------------
#define PCF85063_RTC 0x51
// RTC interrupt:
#define RTC_INT (4 | IO_EXPANDER) // EXIO4 → TCA9555 P04; page 3

// ---------------------------------------------------------------------------
// IMU — QMI8658C (U6) on I2C main bus
// Source: schematic page 3; IMU_INT1=EXIO8
// ---------------------------------------------------------------------------
// #define HAS_QMI8658 1  // TODO: verify if firmware has QMI8658 driver
#define IMU_INT1 (8 | IO_EXPANDER) // EXIO8 → TCA9555 P10; page 3

// ---------------------------------------------------------------------------
// Temperature/Humidity — SHT41 (U11) on I2C main bus, addr 0x44
// Source: schematic page 4; TH_SDA=ESP_SDA, TH_SCL=ESP_SCL
// ---------------------------------------------------------------------------
// SHT41 I2C addr 0x44 confirmed by schematic note "I2C ADDR: 0X44" on page 4.

// ---------------------------------------------------------------------------
// Audio codec — ES8311 (U4) on I2C main bus + I2S
// Source: schematic page 3
// Pins: MCLK=GPIO1, SCLK=GPIO2, ASDOUT=GPIO3, LRCK=GPIO4, DSDIN=GPIO5
// PA_CTRL=EXIO7 (TCA9555 P07)
// HAS_I2S is not enabled here — enabling it requires the ESP8266Audio lib_dep
// and is a future enhancement beyond basic Meshtastic bring-up.
// TODO: add ESP8266Audio + ESP8266SAM lib_deps to platformio.ini and enable
//       HAS_I2S once core radio/GPS/display bring-up is validated on hardware.

// ---------------------------------------------------------------------------
// User inputs
// Source: schematic page 1 — SW1, SW2 (SMD-4P buttons)
// SW1 and SW2 net connections not fully traced from text extraction.
// TODO: verify SW1/SW2 GPIO connections on physical schematic.
// ---------------------------------------------------------------------------
#define BUTTON_PIN 0 // GPIO0 — BOOT/user button (standard ESP32-S3 strapping pin)

// ---------------------------------------------------------------------------
// LEDs
// Source: schematic page 1 (LED4, LED5 BLED), page 3 (LED1 RLED charge indicator,
//         LED2 BLED, LED3 BLED on page 4)
// Charge LED (LED1) is driven by AXP2101 CHGLED pin — not MCU-controlled.
// TODO: verify which GPIO drives LED4/LED5 on page 1.
// ---------------------------------------------------------------------------
// #define LED_POWER   // TODO: verify LED GPIO

// ---------------------------------------------------------------------------
// SPI System-out (peripheral power rail enable)
// Source: schematic page 3; SYS_OUT=EXIO6
// ---------------------------------------------------------------------------
#define SYS_OUT (6 | IO_EXPANDER) // EXIO6 → TCA9555 P06; page 3

// ---------------------------------------------------------------------------
// Flash — W25Q128JVSIQ (U2) — 16 MB, QIO
// Source: schematic page 1; connected to internal SPI flash bus (SPICS0/CLK/D/Q/WP/HD)
// Handled by ESP-IDF / Arduino core partition scheme — no extra defines needed.
// ---------------------------------------------------------------------------

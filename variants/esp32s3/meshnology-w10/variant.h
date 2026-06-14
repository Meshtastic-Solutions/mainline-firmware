#pragma once

// Meshnology W10 — ESP32-S3R8 + EBYTE E22-900MM22S (SX1262) + Quectel L76KB-A58 GPS
// Schematic: W10-MB-Sch-V1_1-0501106.pdf (4 pp., 2025-11-06)

// ─── I2C bus ──────────────────────────────────────────────────────────────────
// Shared by: AXP2101 PMIC, TCA9555 I/O expander, PCF85063ATL RTC,
//            IMU (type TBD), ES8311 codec, SHT41 temp sensor
// Evidence: pg1 net-labels ESP_SCL=GPIO7, ESP_SDA=GPIO8
#define I2C_SDA 8
#define I2C_SCL 7

// ─── Power management ─────────────────────────────────────────────────────────
// AXP2101 PMIC on I2C bus (pg3: AXP_SCL/AXP_SDA tied to ESP_SCL/ESP_SDA)
#define HAS_AXP2101
// AXP_IRQ → EXIO5 (TCA9555 P05) — not a direct ESP32 GPIO
// PMU_IRQ cannot be assigned without custom expander interrupt routing
// TODO: verify AXP2101 IRQ path and implement TCA9555-backed PMU interrupt

// Battery voltage: AXP2101 provides VBAT measurement via I2C — no direct ADC
// TODO: confirm whether any ESP32 GPIO also monitors battery directly

// ─── RTC ──────────────────────────────────────────────────────────────────────
// PCF85063ATL on I2C (pg3: RTC_SCL/RTC_SDA tied to ESP_SCL/ESP_SDA)
// PCF85063 is register-compatible with PCF8563 family
#define PCF8563_RTC 0x51

// ─── LoRa radio ───────────────────────────────────────────────────────────────
// EBYTE E22-900MM22S (SX1262) — pg4 component U10
#define USE_SX1262

// SPI bus — shared with SPI OLED (pg1 OLED_CLK/OLED_MOSI, pg4 E22_SCK/E22_MOSI)
#define LORA_SCK  12  // OLED_CLK / E22_SCK — pg1: OLED_CLK=GPIO12
#define LORA_MOSI 13  // OLED_MOSI / E22_MOSI — pg1: OLED_MOSI=GPIO13
#define LORA_MISO 11  // E22_MISO — pg4: E22_MISO=GPIO11
#define SX126X_CS 14  // E22_NSS (chip-select) — pg4: E22_NSS=GPIO14
#define LORA_CS   SX126X_CS
#define LORA_DIO1 SX126X_DIO1

// CRITICAL: E22 control signals route through TCA9555 I2C expander, NOT direct GPIOs
//   E22_NRST → EXIO3 (TCA9555 P03)  — pg4 net-label "E22_NRST EXIO3"
//   E22_DIO1  → EXIO9 (TCA9555 P11) — pg4 net-label "E22_DI01 EXIO9"
//   E22_BUSY  → EXIO10 (TCA9555 P12) — pg4 net-label "EXIO10 E22_BUSY"
// Set to -1 so the SX1262 driver does not claim phantom GPIO numbers.
// TODO: implement TCA9555-backed LoRa RESET/DIO1/BUSY before this variant
//       can operate with a real radio — these are timing-critical signals.
#define SX126X_RESET -1  // TODO: EXIO3 = TCA9555 port P03
#define SX126X_DIO1  -1  // TODO: EXIO9 = TCA9555 port P11
#define SX126X_BUSY  -1  // TODO: EXIO10 = TCA9555 port P12

// RF switch: E22 TXEN/RXEN wired via E22_RXEN net (pg4, R59=0R, R61=NC)
// TODO: trace E22_RXEN net to the driving GPIO and define SX126X_TXEN/SX126X_RXEN.
//       Until then, tentatively assume DIO2-as-RF-switch internal mode.
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8  // EBYTE E22 series uses 1.8 V TCXO reference
#define SX126X_MAX_POWER 22

// ─── Display ──────────────────────────────────────────────────────────────────
// SPI OLED — shares SPI bus with LoRa (LORA_SCK=GPIO12, LORA_MOSI=GPIO13)
// pg1 net-labels: OLED_CLK=GPIO12, OLED_MOSI=GPIO13, OLED_DC=GPIO16, OLED_CS=GPIO10
// Controller type unconfirmed (schematic labels it "OLED" only)
// TODO: verify actual OLED controller (SSD1306 / SH1106 / SH1107 / other)
#define USE_SPISSD1306
#define SSD1306_NSS   10   // OLED_CS — pg1: LCD_CS=GPIO10
#define SSD1306_RS    16   // OLED_DC (data/command) — pg1: LCD_D/C=GPIO16
// OLED reset: EXIO1 → TCA9555 P01 (pg1: LCD_RST=EXIO1) — not a direct GPIO
#define SSD1306_RESET -1   // TODO: EXIO1 = TCA9555 P01; skip HW reset for now

// pg1: OLED_BL=GPIO6 (may be display power-enable or external LED backlight)
// TODO: confirm OLED_BL function and enable if needed
// #define TFT_BL 6

// ─── GPS ──────────────────────────────────────────────────────────────────────
// Quectel L76KB-A58 on UART0 — pg4 component U9
// pg4 net-labels: GPS_TXD→U0RXD (GPS output → MCU RX), GPS_RXD→U0TXD (MCU TX → GPS)
#define HAS_GPS 1
#define GPS_RX_PIN   44   // U0RXD — MCU receives NMEA from GPS TXD
#define GPS_TX_PIN   43   // U0TXD — MCU sends commands to GPS RXD
#define GPS_BAUDRATE 9600

// pg4: GPS RESET_N → EXIO11 (TCA9555 P13), WAKE_UP → EXIO12 (TCA9555 P14)
// Both route through the TCA9555 expander — not direct GPIOs
// TODO: implement expander-backed GPS RESET and WAKE
// #define GPS_RESET_PIN  // EXIO11 = TCA9555 P13
// #define GPS_WAKE_PIN   // EXIO12 = TCA9555 P14
// 1PPS: connected to "1PPS" net on U9 pin 4 — driving GPIO not confirmed in extracted text
// TODO: identify and define GPS_1PPS_PIN

// ─── User input ───────────────────────────────────────────────────────────────
// pg1: SW2 (SMD 4-pin) pulls GPIO0 to GND — standard ESP32 boot/user button
#define BUTTON_PIN 0

// ─── Status LED ───────────────────────────────────────────────────────────────
// pg3 shows LED1 (RLED), LED2 (BLED), LED3 (BLED) driven via TCA9555 nets
// TODO: trace LED net labels to GPIO/expander pins and define LED_POWER
// #define LED_POWER

// ─── On-board peripherals (not wired up in this initial scaffold) ─────────────
// TCA9555 I/O expander: I2C address 0x20 (assume A0=A1=A2=GND — pg3, verify)
// SHT41 temp/humidity sensor: I2C address 0x44 (pg4: TH_SDA/SCL tied to ESP bus)
// ES8311 audio codec + NS4150 amplifier: I2S GPIO1-5 (pg3) — not enabled
// IMU (part number unknown, INT1→EXIO8): I2C bus — not enabled
// Camera interface: GPIO38-42, GPIO45-48 (pg1) — not enabled

// ─── Board identity ───────────────────────────────────────────────────────────
#define MESHNOLOGY_W10 1

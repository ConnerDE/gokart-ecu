/*
 * ESP32-S3 Unterbau-Controller (ESP2)
 * VERSION: v7.0 FINAL (Calibration & Learning Mode)
 * STATUS: FULL CODE – COMPLETE
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <driver/pulse_cnt.h>
#include <Preferences.h>
#include <driver/ledc.h>
#include <driver/twai.h>
#include <TMCStepper.h>
#include <esp_task_wdt.h>

/* ==================== DEFINITIONS & ENUMS ==================== */
enum StatusLEDMode { LED_NORMAL, LED_EMERGENCY, LED_CAN_LOSS, LED_CALIBRATION };

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a7"


/* ==================== CONSTANTS ==================== */
#define MAX_GEAR              5
#define RPM_SHIFT_LOCK        3500
#define BATTERY_CUTOFF        10.5
#define BATTERY_FACTOR        4.895
#define OIL_TEMP_CRITICAL     110.0
#define BATTERY_SAMPLES       10
#define CAN_TIMEOUT_MS        2000
#define IGNITION_THRESHOLD    9.0
#define GAS_MAX_STEPS_RAW     100 // Interner Zähler-Maximalwert (vor Kalibrierung)
#define SERVICE_HOURS_LIMIT   10.0
#define CHAIN_SHIFTS_LIMIT    50
#define WDT_TIMEOUT           10

/* ==================== LEDC ==================== */
#define LEDC_SERVO_EXH_CH    LEDC_CHANNEL_0
#define LEDC_SERVO_GAS_CH    LEDC_CHANNEL_1
#define LEDC_SERVO_TIMER     LEDC_TIMER_0
#define LEDC_SERVO_MODE      LEDC_LOW_SPEED_MODE
#define LEDC_SERVO_FREQ      50
#define LEDC_SERVO_RES       LEDC_TIMER_13_BIT
#define LEDC_SERVO_DUTY_MAX  8191

/* ==================== EEPROM KEYS ==================== */
#define PREF_NAMESPACE    "esp2"
#define PREF_GEAR         "gear"
#define PREF_SHIFTS       "shifts"
#define PREF_CHAIN        "chain"
#define PREF_HOURS        "hours"
// Calibration Keys
#define PREF_GAS_MIN      "gas_min"
#define PREF_GAS_MAX      "gas_max"
#define PREF_SRV_GAS_MIN  "srv_g_min"
#define PREF_SRV_GAS_MAX  "srv_g_max"
#define PREF_SRV_EXH_MIN  "srv_e_min"
#define PREF_SRV_EXH_MAX  "srv_e_max"

/* ==================== GLOBAL OBJECTS ==================== */
Adafruit_MCP23X17 mcp1, mcp2;
Adafruit_AHTX0 aht20;
Adafruit_BMP280 bmp280;
OneWire oneWire(PIN_OIL_TEMP);
DallasTemperature ds18b20(&oneWire);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences prefs;
TMC2209Stepper driver(&Serial1, R_SENSE, DRIVER_ADDRESS);

CRGB ledsFrontVL[NUM_FRONT_LEDS];
CRGB ledsFrontVR[NUM_FRONT_LEDS];
CRGB ledsHeckHL[NUM_HECK_HL];
CRGB ledsHeckHR[NUM_HECK_HR];
CRGB ledsSpoiler[NUM_SPOILER];

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
BLECharacteristic* pCharInput = nullptr;
bool deviceConnected = false;
bool usbState = false;

uint32_t totalShifts = 0;
uint32_t chainShifts = 0;
float engineHours = 0.0;
float currentSpeed = 0.0;
uint8_t i2cStatus = 0;

// Calibration Variables
int32_t calGasMin = 0;
int32_t calGasMax = 60;
int calSrvGasMin = 0;
int calSrvGasMax = 180;
int calSrvExhMin = 0;
int calSrvExhMax = 180;

/* ==================== INSTANCES ==================== */
RPMCounter rpm;
CanReceiver can;
SafetyModule safety;
Gearbox gearbox;
GasPedal gasPedal;
LightsManager lights;

bool blinkerLeftState = false;
bool blinkerRightState = false;
bool warnBlinkState = false;
bool lastBlinkLeftBtn = false;
bool lastBlinkRightBtn = false;
bool lastWarnBtn = false;
unsigned long blinkerStartTime = 0;


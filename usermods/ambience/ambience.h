#pragma once

#define DEBUG 0  // Enable debugging to log configuration and parsing

#if DEBUG
  #define SERIAL_PRINT(x) Serial.print(x)
  #define SERIAL_PRINTLN(x) Serial.println(x)
  #define SERIAL_PRINTF(x, ...) Serial.printf(x, ##__VA_ARGS__)
#else
  #define SERIAL_PRINT(x)
  #define SERIAL_PRINTLN(x)
  #define SERIAL_PRINTF(x, ...)
#endif

#include "wled.h"
#include <esp_now.h>
#include <WiFi.h>

// Global pointer to the usermod instance (standard v2 pattern)
class AmbienceV1;
AmbienceV1* ambienceInstance = nullptr;

class AmbienceV1 : public Usermod {
private:
  bool enabled = true;
  bool initDone = false;
  bool UseESPNow = true;
  bool UseBlindSpot = true;
  bool UseBrakeLight = true;
  bool UseVehicleSWC = true;
  bool UseFrontOccupancy = true;
  bool UseRearOccupancy = true;
  bool UseCollisionWarning = true;  
  bool UseCarWashMode = true;
  bool UseNightBrightnessCap = true;    // When dark outside, cap LED brightness to avoid car wash blowout
  float nightBrightnessCapPct = 60.0;  // Max display brightness % allowed when is_sun_up == 0

  // ESP-NOW received data (Chassis Bus)
  uint8_t blind_spot_left = 0;
  uint8_t blind_spot_right = 0;
  uint8_t forward_collision = 0;
  uint8_t is_sun_up = 0;
  uint16_t vehicle_speed_raw = 0;
  float vehicle_speed_mph = 0.0;
  
  // ESP-NOW received data (Vehicle Bus)
  float display_brightness_level = 0.0;
  uint8_t ambient_light_enabled = 0;
  uint8_t brake_status = 0;
  uint8_t turn_indicator_stalk_status = 0;
  uint8_t turn_signal_left_status = 0;
  uint8_t turn_signal_right_status = 0;
  uint8_t swc_left_held = 0;
  uint8_t swc_left_double_press = 0;
  uint8_t swc_left_tilt_left_held = 0;
  uint8_t swc_left_tilt_right_held = 0;
  uint8_t swc_right_held = 0;
  uint8_t swc_right_double_press = 0;
  uint8_t swc_right_tilt_left_held = 0;
  uint8_t swc_right_tilt_right_held = 0;
  uint8_t front_occupancy = 0;
  uint8_t rear_occupancy = 0;
  uint8_t car_wash_mode_request = 0; 

  // Previous values for change detection
  float prev_display_brightness_level = 0.0;
  uint8_t prev_ambient_light_enabled = 0;
  uint8_t prev_brake_status = 0;
  uint8_t prev_turn_indicator_stalk_status = 0;
  uint8_t prev_turn_signal_left_status = 0;
  uint8_t prev_turn_signal_right_status = 0;
  uint8_t prev_blind_spot_left = 0;
  uint8_t prev_blind_spot_right = 0;
  uint8_t prev_forward_collision = 0;
  uint8_t prev_front_occupancy = 0;
  uint8_t prev_rear_occupancy = 0;
  uint8_t prev_is_sun_up = 0;
  uint8_t prev_swc_left_held = 0;
  uint8_t prev_swc_left_double_press = 0;
  uint8_t prev_swc_left_tilt_left_held = 0;
  uint8_t prev_swc_left_tilt_right_held = 0;
  uint8_t prev_swc_right_held = 0;
  uint8_t prev_swc_right_double_press = 0;
  uint8_t prev_swc_right_tilt_left_held = 0;
  uint8_t prev_swc_right_tilt_right_held = 0;
  uint8_t prev_car_wash_mode_request = 0; 

  // Preset definitions
  uint16_t PRST_1_10 = 30;
  uint16_t PRST_11_20 = 31;
  uint16_t PRST_21_30 = 35;
  uint16_t PRST_31_40 = 36;
  uint16_t PRST_41_50 = 37;
  uint16_t PRST_51_60 = 38;
  uint16_t PRST_61_70 = 40;
  uint16_t PRST_71_80 = 42;
  uint16_t PRST_81_90 = 43;
  uint16_t PRST_91_100 = 44;
  uint16_t PRST_SWC_LEFT_HELD = 1;
  uint16_t PRST_SWC_LEFT_DOUBLE_PRESS = 2;
  uint16_t PRST_SWC_LEFT_TILT_LEFT = 3;
  uint16_t PRST_SWC_LEFT_TILT_RIGHT = 4;
  uint16_t PRST_SWC_RIGHT_HELD = 5;
  uint16_t PRST_SWC_RIGHT_DOUBLE_PRESS = 6;
  uint16_t PRST_SWC_RIGHT_TILT_LEFT = 7;
  uint16_t PRST_SWC_RIGHT_TILT_RIGHT = 8;
  uint16_t PRST_FRONT_OCCUPANCY_ON = 13;
  uint16_t PRST_FRONT_OCCUPANCY_OFF = 14;
  uint16_t PRST_REAR_OCCUPANCY_ON = 9;
  uint16_t PRST_REAR_OCCUPANCY_OFF = 10;
  uint16_t PRST_CAR_WASH_MODE_ON = 11; 
  uint16_t PRST_CAR_WASH_MODE_OFF = 12; 
  uint16_t display_new_preset = 0;
  uint16_t display_prev_preset = 0;
  String Status;

  // Configurable LED ranges
  uint16_t brakeLightStart = 130;
  uint16_t brakeLightStop = 160;
  uint16_t leftBlindSpotStart = 30;
  uint16_t leftBlindSpotStop = 80;
  uint16_t rightBlindSpotStart = 210;
  uint16_t rightBlindSpotStop = 250;
  uint16_t collisionWarningStart = 2;
  uint16_t collisionWarningStop = 320;

  // Dynamic LED color arrays ( Use total number of LED Light's)
  uint32_t leftBlindSpotColors[495];
  uint32_t rightBlindSpotColors[495];
  uint32_t brakeLightColors[495];
  uint32_t collisionWarningColors[495];
  bool blindSpotActiveLeft = false;
  bool blindSpotActiveRight = false;
  bool brakeLightActive = false;
  bool collisionWarningActive = false;

  // Turn signal debounce timers ( Used so blind spot does not blink when turn signal is blinking)
  unsigned long leftTurnSignalOffTime = 0;
  unsigned long rightTurnSignalOffTime = 0;
  const unsigned long TURN_SIGNAL_DELAY = 500;
  bool leftTurnSignalRedActive = false;
  bool rightTurnSignalRedActive = false;

  // Blind spot RGB color
  struct {
    // Default: Orange
    uint8_t r = 255;  
    uint8_t g = 140;
    uint8_t b = 0;
  } blindSpotRGB;

  // Configuration strings
  static constexpr char _name[] PROGMEM = "Ambience";
  static constexpr char _enabled[] PROGMEM = "Enabled";
  static constexpr char _UseESPNow[] PROGMEM = "USE ESPNow";
  static constexpr char _UseBlindSpot[] PROGMEM = "USE BlindSpot";
  static constexpr char _UseBrakeLight[] PROGMEM = "USE BrakeLight";
  static constexpr char _UseVehicleSWC[] PROGMEM = "USE VehicleSWC";
  static constexpr char _UseFrontOccupancy[] PROGMEM = "USE FrontOccupancy";
  static constexpr char _UseRearOccupancy[] PROGMEM = "USE RearOccupancy";
  static constexpr char _UseCollisionWarning[] PROGMEM = "USE CollisionWarning";
  static constexpr char _UseCarWashMode[] PROGMEM = "USE CarWashMode";
  static constexpr char _UseNightBrightnessCap[] PROGMEM = "USE NightBrightnessCap";
  static constexpr char _nightBrightnessCapPct[] PROGMEM = "Night Max Brightness %";
  static constexpr char _brakeLightStart[] PROGMEM = "Brake Light Start LED";
  static constexpr char _brakeLightStop[] PROGMEM = "Brake Light Stop LED";
  static constexpr char _leftBlindSpotStart[] PROGMEM = "Left Blind Spot Start LED";
  static constexpr char _leftBlindSpotStop[] PROGMEM = "Left Blind Spot Stop LED";
  static constexpr char _rightBlindSpotStart[] PROGMEM = "Right Blind Spot Start LED";
  static constexpr char _rightBlindSpotStop[] PROGMEM = "Right Blind Spot Stop LED";
  static constexpr char _collisionWarningStart[] PROGMEM = "Collision Warning Start LED";
  static constexpr char _collisionWarningStop[] PROGMEM = "Collision Warning Stop LED";
  static constexpr char _blindSpotRGB[] PROGMEM = "Blind Spot RGB (R,G,B)";
  static constexpr char _PRST_1_10[] PROGMEM = "1-10% Preset";
  static constexpr char _PRST_11_20[] PROGMEM = "11-20% Preset";
  static constexpr char _PRST_21_30[] PROGMEM = "21-30% Preset";
  static constexpr char _PRST_31_40[] PROGMEM = "31-40% Preset";
  static constexpr char _PRST_41_50[] PROGMEM = "41-50% Preset";
  static constexpr char _PRST_51_60[] PROGMEM = "51-60% Preset";
  static constexpr char _PRST_61_70[] PROGMEM = "61-70% Preset";
  static constexpr char _PRST_71_80[] PROGMEM = "71-80% Preset";
  static constexpr char _PRST_81_90[] PROGMEM = "81-90% Preset";
  static constexpr char _PRST_91_100[] PROGMEM = "91-100% Preset";
  static constexpr char _PRST_SWC_LEFT_HELD[] PROGMEM = "Left Held Preset";
  static constexpr char _PRST_SWC_LEFT_DOUBLE_PRESS[] PROGMEM = "Left Double Press Preset";
  static constexpr char _PRST_SWC_LEFT_TILT_LEFT[] PROGMEM = "Left Tilt Left Preset";
  static constexpr char _PRST_SWC_LEFT_TILT_RIGHT[] PROGMEM = "Left Tilt Right Preset";
  static constexpr char _PRST_SWC_RIGHT_HELD[] PROGMEM = "Right Held Preset";
  static constexpr char _PRST_SWC_RIGHT_DOUBLE_PRESS[] PROGMEM = "Right Double Press Preset";
  static constexpr char _PRST_SWC_RIGHT_TILT_LEFT[] PROGMEM = "Right Tilt Left Preset";
  static constexpr char _PRST_SWC_RIGHT_TILT_RIGHT[] PROGMEM = "Right Tilt Right Preset";
  static constexpr char _PRST_FRONT_OCCUPANCY_ON[] PROGMEM = "Front Occupancy ON Preset";
  static constexpr char _PRST_FRONT_OCCUPANCY_OFF[] PROGMEM = "Front Occupancy OFF Preset";
  static constexpr char _PRST_REAR_OCCUPANCY_ON[] PROGMEM = "Rear Occupancy ON Preset";
  static constexpr char _PRST_REAR_OCCUPANCY_OFF[] PROGMEM = "Rear Occupancy OFF Preset";
  static constexpr char _PRST_CAR_WASH_MODE_ON[] PROGMEM = "Car Wash Mode ON Preset"; 
  static constexpr char _PRST_CAR_WASH_MODE_OFF[] PROGMEM = "Car Wash Mode OFF Preset"; 

public:
  // Structure to hold vehicle CAN data
  // IF CAN BUS STRUCT SIZE IS WRONG AND DOES NOT MATCH IT WILL NOT WORK !!!!
#pragma pack(push, 1)
struct vehicle_can_struct {
  uint8_t sender_id;                    //1 bytes ID:1 for Vehicle  
  float display_brightness_level;       // 4 bytes
  uint8_t ambient_light_enabled;        // 1 byte
  uint8_t brake_status;                 // 1 byte
  uint8_t turn_indicator_stalk_status;  // 1 byte
  uint8_t turn_signal_left_status;      // 1 byte
  uint8_t turn_signal_right_status;     // 1 byte
  uint8_t swc_left_held;                // 1 byte
  uint8_t swc_left_double_press;        // 1 byte
  uint8_t swc_left_tilt_left_held;      // 1 byte
  uint8_t swc_left_tilt_right_held;     // 1 byte
  uint8_t swc_right_held;               // 1 byte
  uint8_t swc_right_double_press;       // 1 byte
  uint8_t swc_right_tilt_left_held;     // 1 byte
  uint8_t swc_right_tilt_right_held;    // 1 byte
  uint8_t front_occupancy;              // 1 byte
  uint8_t rear_occupancy;               // 1 byte
  uint8_t car_wash_mode_request;        // 1 byte
}; // Size: 21 bytes
#pragma pack(pop)

  // Structure to hold chassis CAN data
  // IF CAN BUS STRUCT SIZE IS WRONG AND DOES NOT MATCH IT WILL NOT WORK !!!!
#pragma pack(push, 1)
struct chassis_can_struct {
  uint8_t sender_id;          // 1 bytes ID:0 for Chassis  
  uint8_t blind_spot_left;    // 1 bytes
  uint8_t blind_spot_right;   // 1 bytes
  uint8_t forward_collision;  // 1 bytes
  uint8_t is_sun_up;          // 1 bytes
  uint16_t vehicle_speed_raw; // 2 bytes
  float vehicle_speed_mph;    // 4 bytes
}; // Size: 11 bytes
#pragma pack(pop)

  void setup() override {
    ambienceInstance = this;  // required for static OnDataRecv callback

    #if DEBUG
      Serial.begin(115200);
      while (!Serial);
    
      SERIAL_PRINT("Chassis data struct size: "); SERIAL_PRINTLN(sizeof(chassis_can_struct));
      SERIAL_PRINT("Vehicle data struct size: "); SERIAL_PRINTLN(sizeof(vehicle_can_struct));
      /*
      if (sizeof(chassis_can_struct) != 11 || sizeof(vehicle_can_struct) != 21) {
        SERIAL_PRINTLN("Error: Struct size mismatch, expected 11 bytes for chassis, 21 bytes for vehicle");
        while (true); // Halt to prevent data corruption
      }
      */
    #endif

    initDone = true;
    if (UseESPNow) {
      WiFi.mode(WIFI_STA);
      if (esp_now_init() != ESP_OK) {
        SERIAL_PRINTLN("ESP-NOW Init Failed");
      } else {
        esp_now_register_recv_cb(OnDataRecv);
      }
    }
  }

  void connected() override {
    SERIAL_PRINTLN("WLED Setup Complete, Awaiting Wi-Fi");
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
      SERIAL_PRINTLN("ESP-NOW Init Failed");
      return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    SERIAL_PRINTLN("ESP-NOW Initialized in AmbienceV1");
    SERIAL_PRINTLN("MAC Address: " + WiFi.macAddress());
    SERIAL_PRINTLN(WiFi.localIP());
    SERIAL_PRINTLN("WiFi Mode: " + String(WiFi.getMode()));
  }

  static void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    #if DEBUG
      SERIAL_PRINTLN("--------------------------------------------");
      SERIAL_PRINTLN("ESP-NOW Data Received! Length: " + String(len));
      SERIAL_PRINT("Raw packet bytes: ");
      for (int i = 0; i < len; i++) {
        SERIAL_PRINTF("%02X ", incomingData[i]);
      }
      SERIAL_PRINTLN();
    #endif

    if (!ambienceInstance) return;

    uint8_t sender_id = incomingData[0];

    if (sender_id == 0 && len == sizeof(chassis_can_struct)) {
      chassis_can_struct receivedData;
      memcpy(&receivedData, incomingData, sizeof(receivedData));
      ambienceInstance->blind_spot_left = receivedData.blind_spot_left;
      ambienceInstance->blind_spot_right = receivedData.blind_spot_right;
      ambienceInstance->forward_collision = receivedData.forward_collision;
      ambienceInstance->is_sun_up = receivedData.is_sun_up;
      ambienceInstance->vehicle_speed_raw = receivedData.vehicle_speed_raw;
      ambienceInstance->vehicle_speed_mph = receivedData.vehicle_speed_mph;
      
      #if DEBUG
        SERIAL_PRINT("blind_spot_left: "); SERIAL_PRINTLN(ambienceInstance->blind_spot_left);
        SERIAL_PRINT("blind_spot_right: "); SERIAL_PRINTLN(ambienceInstance->blind_spot_right);
        SERIAL_PRINT("forward_collision: "); SERIAL_PRINTLN(ambienceInstance->forward_collision);
        SERIAL_PRINT("is_sun_up: "); SERIAL_PRINTLN(ambienceInstance->is_sun_up);
        SERIAL_PRINT("vehicle_speed_raw: "); SERIAL_PRINTLN(ambienceInstance->vehicle_speed_raw);
        SERIAL_PRINT("vehicle_speed_mph: "); SERIAL_PRINTLN(ambienceInstance->vehicle_speed_mph);
      #endif
    }
    else if (sender_id == 1 && len == sizeof(vehicle_can_struct)) {
      vehicle_can_struct receivedData;
      memcpy(&receivedData, incomingData, sizeof(receivedData));
      ambienceInstance->display_brightness_level = receivedData.display_brightness_level;
      ambienceInstance->ambient_light_enabled = receivedData.ambient_light_enabled;
      ambienceInstance->brake_status = receivedData.brake_status;
      ambienceInstance->turn_indicator_stalk_status = receivedData.turn_indicator_stalk_status;
      ambienceInstance->turn_signal_left_status = receivedData.turn_signal_left_status;
      ambienceInstance->turn_signal_right_status = receivedData.turn_signal_right_status;
      ambienceInstance->swc_left_held = receivedData.swc_left_held;
      ambienceInstance->swc_left_double_press = receivedData.swc_left_double_press;
      ambienceInstance->swc_left_tilt_left_held = receivedData.swc_left_tilt_left_held;
      ambienceInstance->swc_left_tilt_right_held = receivedData.swc_left_tilt_right_held;
      ambienceInstance->swc_right_held = receivedData.swc_right_held;
      ambienceInstance->swc_right_double_press = receivedData.swc_right_double_press;
      ambienceInstance->swc_right_tilt_left_held = receivedData.swc_right_tilt_left_held;
      ambienceInstance->swc_right_tilt_right_held = receivedData.swc_right_tilt_right_held;
      ambienceInstance->front_occupancy = receivedData.front_occupancy;
      ambienceInstance->rear_occupancy = receivedData.rear_occupancy;
      ambienceInstance->car_wash_mode_request = receivedData.car_wash_mode_request; 

      #if DEBUG
        SERIAL_PRINT("display_brightness_level: "); SERIAL_PRINTLN(ambienceInstance->display_brightness_level);
        SERIAL_PRINT("ambient_light_enabled: "); SERIAL_PRINTLN(ambienceInstance->ambient_light_enabled);
        SERIAL_PRINT("brake_status: "); SERIAL_PRINTLN(ambienceInstance->brake_status);
        SERIAL_PRINT("turn_indicator_stalk_status: "); SERIAL_PRINTLN(ambienceInstance->turn_indicator_stalk_status);
        SERIAL_PRINT("turn_signal_left_status: "); SERIAL_PRINTLN(ambienceInstance->turn_signal_left_status);
        SERIAL_PRINT("turn_signal_right_status: "); SERIAL_PRINTLN(ambienceInstance->turn_signal_right_status);
        SERIAL_PRINT("swc_left_held: "); SERIAL_PRINTLN(ambienceInstance->swc_left_held);
        SERIAL_PRINT("swc_left_double_press: "); SERIAL_PRINTLN(ambienceInstance->swc_left_double_press);
        SERIAL_PRINT("swc_left_tilt_left_held: "); SERIAL_PRINTLN(ambienceInstance->swc_left_tilt_left_held);
        SERIAL_PRINT("swc_left_tilt_right_held: "); SERIAL_PRINTLN(ambienceInstance->swc_left_tilt_right_held);
        SERIAL_PRINT("swc_right_held: "); SERIAL_PRINTLN(ambienceInstance->swc_right_held);
        SERIAL_PRINT("swc_right_double_press: "); SERIAL_PRINTLN(ambienceInstance->swc_right_double_press);
        SERIAL_PRINT("swc_right_tilt_left_held: "); SERIAL_PRINTLN(ambienceInstance->swc_right_tilt_left_held);
        SERIAL_PRINT("swc_right_tilt_right_held: "); SERIAL_PRINTLN(ambienceInstance->swc_right_tilt_right_held);
        SERIAL_PRINT("front_occupancy: "); SERIAL_PRINTLN(ambienceInstance->front_occupancy ? "YES" : "NO");
        SERIAL_PRINT("rear_occupancy: "); SERIAL_PRINTLN(ambienceInstance->rear_occupancy ? "YES" : "NO");
        SERIAL_PRINT("car_wash_mode_request: "); SERIAL_PRINTLN(ambienceInstance->car_wash_mode_request ? "YES" : "NO");
      #endif
    }
  }

  // All functions below are 100% unchanged from your original file
  void EspNowHandling() {
    // Brightness presets (Does not change brightness when in carwash mode, Keep it bright)
    if (!car_wash_mode_request && display_brightness_level != prev_display_brightness_level) {

      // Night brightness cap: when it's dark outside, clamp the display brightness
      // so car wash lights can't blast the interior LEDs to full brightness
      float effectiveBrightness = display_brightness_level;
      if (UseNightBrightnessCap && is_sun_up == 0) {
        if (effectiveBrightness > nightBrightnessCapPct) {
          effectiveBrightness = nightBrightnessCapPct;
          SERIAL_PRINTLN("Night brightness cap applied: " + String(display_brightness_level) + "% -> " + String(effectiveBrightness) + "%");
        }
      }

      if (effectiveBrightness > 0 && effectiveBrightness <= 10) display_new_preset = 1;
      else if (effectiveBrightness > 10 && effectiveBrightness <= 20) display_new_preset = 2;
      else if (effectiveBrightness > 20 && effectiveBrightness <= 30) display_new_preset = 3;
      else if (effectiveBrightness > 30 && effectiveBrightness <= 40) display_new_preset = 4;
      else if (effectiveBrightness > 40 && effectiveBrightness <= 50) display_new_preset = 5;
      else if (effectiveBrightness > 50 && effectiveBrightness <= 60) display_new_preset = 6;
      else if (effectiveBrightness > 60 && effectiveBrightness <= 70) display_new_preset = 7;
      else if (effectiveBrightness > 70 && effectiveBrightness <= 80) display_new_preset = 8;
      else if (effectiveBrightness > 80 && effectiveBrightness <= 90) display_new_preset = 9;
      else if (effectiveBrightness > 90 && effectiveBrightness <= 100) display_new_preset = 10;
      else display_new_preset = 0;

      prev_display_brightness_level = display_brightness_level;

      if (display_new_preset != display_prev_preset) {
        switch (display_new_preset) {
          case 1: applyPreset(PRST_1_10, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 1-10%"; break;
          case 2: applyPreset(PRST_11_20, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 11-20%"; break;
          case 3: applyPreset(PRST_21_30, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 21-30%"; break;
          case 4: applyPreset(PRST_31_40, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 31-40%"; break;
          case 5: applyPreset(PRST_41_50, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 41-50%"; break;
          case 6: applyPreset(PRST_51_60, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 51-60%"; break;
          case 7: applyPreset(PRST_61_70, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 61-70%"; break;
          case 8: applyPreset(PRST_71_80, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 71-80%"; break;
          case 9: applyPreset(PRST_81_90, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 81-90%"; break;
          case 10: applyPreset(PRST_91_100, CALL_MODE_BUTTON_PRESET); Status = "Brightness Level: 91-100%"; break;
          default: Status = "Brightness Level: Invalid"; break;
        }
        display_prev_preset = display_new_preset;
      }
    }
// Vehicle Steering Wheel Controls
    if (UseVehicleSWC) {
      if (swc_left_double_press != prev_swc_left_double_press) {
        SERIAL_PRINTLN("swc_left_double_press changed to: " + String(swc_left_double_press));
        if (swc_left_double_press == 1) {
          if (PRST_SWC_LEFT_DOUBLE_PRESS > 0 && PRST_SWC_LEFT_DOUBLE_PRESS <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_LEFT_DOUBLE_PRESS: " + String(PRST_SWC_LEFT_DOUBLE_PRESS));
            applyPreset(PRST_SWC_LEFT_DOUBLE_PRESS, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_left_double_press ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_left_double_press: " + String(PRST_SWC_LEFT_DOUBLE_PRESS));
          }
        }
        prev_swc_left_double_press = swc_left_double_press;
      }
      if (swc_left_held != prev_swc_left_held) {
        SERIAL_PRINTLN("swc_left_held changed to: " + String(swc_left_held));
        if (swc_left_held == 1) {
          if (PRST_SWC_LEFT_HELD > 0 && PRST_SWC_LEFT_HELD <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_LEFT_HELD: " + String(PRST_SWC_LEFT_HELD));
            applyPreset(PRST_SWC_LEFT_HELD, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_left_held ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_left_held: " + String(PRST_SWC_LEFT_HELD));
          }
        }
        prev_swc_left_held = swc_left_held;
      }
      if (swc_left_tilt_left_held != prev_swc_left_tilt_left_held) {
        SERIAL_PRINTLN("swc_left_tilt_left_held changed to: " + String(swc_left_tilt_left_held));
        if (swc_left_tilt_left_held == 1) {
          if (PRST_SWC_LEFT_TILT_LEFT > 0 && PRST_SWC_LEFT_TILT_LEFT <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_LEFT_TILT_LEFT: " + String(PRST_SWC_LEFT_TILT_LEFT));
            applyPreset(PRST_SWC_LEFT_TILT_LEFT, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_left_tilt_left_held ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_left_tilt_left_held: " + String(PRST_SWC_LEFT_TILT_LEFT));
          }
        }
        prev_swc_left_tilt_left_held = swc_left_tilt_left_held;
      }
      if (swc_left_tilt_right_held != prev_swc_left_tilt_right_held) {
        SERIAL_PRINTLN("swc_left_tilt_right_held changed to: " + String(swc_left_tilt_right_held));
        if (swc_left_tilt_right_held == 1) {
          if (PRST_SWC_LEFT_TILT_RIGHT > 0 && PRST_SWC_LEFT_TILT_RIGHT <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_LEFT_TILT_RIGHT: " + String(PRST_SWC_LEFT_TILT_RIGHT));
            applyPreset(PRST_SWC_LEFT_TILT_RIGHT, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_left_tilt_right_held ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_left_tilt_right_held: " + String(PRST_SWC_LEFT_TILT_RIGHT));
          }
        }
        prev_swc_left_tilt_right_held = swc_left_tilt_right_held;
      }
      if (swc_right_double_press != prev_swc_right_double_press) {
        SERIAL_PRINTLN("swc_right_double_press changed to: " + String(swc_right_double_press));
        if (swc_right_double_press == 1) {
          if (PRST_SWC_RIGHT_DOUBLE_PRESS > 0 && PRST_SWC_RIGHT_DOUBLE_PRESS <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_RIGHT_DOUBLE_PRESS: " + String(PRST_SWC_RIGHT_DOUBLE_PRESS));
            applyPreset(PRST_SWC_RIGHT_DOUBLE_PRESS, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_right_double_press ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_right_double_press: " + String(PRST_SWC_RIGHT_DOUBLE_PRESS));
          }
        }
        prev_swc_right_double_press = swc_right_double_press;
      }
      if (swc_right_held != prev_swc_right_held) {
        SERIAL_PRINTLN("swc_right_held changed to: " + String(swc_right_held));
        if (swc_right_held == 1) {
          if (PRST_SWC_RIGHT_HELD > 0 && PRST_SWC_RIGHT_HELD <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_RIGHT_HELD: " + String(PRST_SWC_RIGHT_HELD));
            applyPreset (PRST_SWC_RIGHT_HELD, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_right_held ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_right_held: " + String(PRST_SWC_RIGHT_HELD));
          }
        }
        prev_swc_right_held = swc_right_held;
      }
      if (swc_right_tilt_left_held != prev_swc_right_tilt_left_held) {
        SERIAL_PRINTLN("swc_right_tilt_left_held changed to: " + String(swc_right_tilt_left_held));
        if (swc_right_tilt_left_held == 1) {
          if (PRST_SWC_RIGHT_TILT_LEFT > 0 && PRST_SWC_RIGHT_TILT_LEFT <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_RIGHT_TILT_LEFT: " + String(PRST_SWC_RIGHT_TILT_LEFT));
            applyPreset(PRST_SWC_RIGHT_TILT_LEFT, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_right_tilt_left_held ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_right_tilt_left_held: " + String(PRST_SWC_RIGHT_TILT_LEFT));
          }
        }
        prev_swc_right_tilt_left_held = swc_right_tilt_left_held;
      }
      if (swc_right_tilt_right_held != prev_swc_right_tilt_right_held) {
        SERIAL_PRINTLN("swc_right_tilt_right_held changed to: " + String(swc_right_tilt_right_held));
        if (swc_right_tilt_right_held == 1) {
          if (PRST_SWC_RIGHT_TILT_RIGHT > 0 && PRST_SWC_RIGHT_TILT_RIGHT <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_SWC_RIGHT_TILT_RIGHT: " + String(PRST_SWC_RIGHT_TILT_RIGHT));
            applyPreset(PRST_SWC_RIGHT_TILT_RIGHT, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "swc_right_tilt_right_held ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for swc_right_tilt_right_held: " + String(PRST_SWC_RIGHT_TILT_RIGHT));
          }
        }
        prev_swc_right_tilt_right_held = swc_right_tilt_right_held;
      }
    }
    //
    // Rear Occupancy  ( Used to turn on and off rear seat LED's if noone is in the back)
    if (UseFrontOccupancy) {
      if (front_occupancy != prev_front_occupancy) {
        if (front_occupancy) {
          applyPreset(PRST_FRONT_OCCUPANCY_ON, CALL_MODE_BUTTON_PRESET);
          Status = "Rear Occupancy ON";
        } else {
          applyPreset(PRST_FRONT_OCCUPANCY_OFF, CALL_MODE_BUTTON_PRESET);
          Status = "Rear Occupancy OFF";
        }
        prev_front_occupancy = front_occupancy;
      }
    }

    // Rear Occupancy  ( Used to turn on and off rear seat LED's if noone is in the back)
    if (UseRearOccupancy) {
      if (rear_occupancy != prev_rear_occupancy) {
        if (rear_occupancy) {
          applyPreset(PRST_REAR_OCCUPANCY_ON, CALL_MODE_BUTTON_PRESET);
          Status = "Rear Occupancy ON";
        } else {
          applyPreset(PRST_REAR_OCCUPANCY_OFF, CALL_MODE_BUTTON_PRESET);
          Status = "Rear Occupancy OFF";
        }
        prev_rear_occupancy = rear_occupancy;
      }
    }

    // Car Wash Mode 
    if (UseCarWashMode) { 
      if (car_wash_mode_request != prev_car_wash_mode_request) {
        SERIAL_PRINTLN("car_wash_mode_request changed to: " + String(car_wash_mode_request));
        if (car_wash_mode_request == 1) {
          if (PRST_CAR_WASH_MODE_ON > 0 && PRST_CAR_WASH_MODE_ON <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_CAR_WASH_MODE: " + String(PRST_CAR_WASH_MODE_ON));
            applyPreset(PRST_CAR_WASH_MODE_ON, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "Car Wash Mode ON";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for car_wash_mode_request: " + String(PRST_CAR_WASH_MODE_ON));
          }
        }
        else if (car_wash_mode_request == 0) {
          if (PRST_CAR_WASH_MODE_OFF > 0 && PRST_CAR_WASH_MODE_OFF <= 255) {
            SERIAL_PRINTLN("Applying preset PRST_CAR_WASH_MODE: " + String(PRST_CAR_WASH_MODE_OFF));
            applyPreset(PRST_CAR_WASH_MODE_OFF, CALL_MODE_BUTTON_PRESET);
            colorUpdated(CALL_MODE_NOTIFICATION); // Force refresh
            Status = "Car Wash Mode OFF";
          } else {
            SERIAL_PRINTLN("Invalid preset ID for car_wash_mode_request: " + String(PRST_CAR_WASH_MODE_OFF));
          }
        }
        prev_car_wash_mode_request = car_wash_mode_request;
      }
    }

    // Update previous values for other signals
    if (brake_status != prev_brake_status) prev_brake_status = brake_status;
    if (forward_collision != prev_forward_collision) prev_forward_collision = forward_collision;
    if (turn_indicator_stalk_status != prev_turn_indicator_stalk_status) prev_turn_indicator_stalk_status = turn_indicator_stalk_status;
    if (ambient_light_enabled != prev_ambient_light_enabled) prev_ambient_light_enabled = ambient_light_enabled;
    if (blind_spot_left != prev_blind_spot_left) prev_blind_spot_left = blind_spot_left;
    if (blind_spot_right != prev_blind_spot_right) prev_blind_spot_right = blind_spot_right;
    if (turn_signal_left_status != prev_turn_signal_left_status) prev_turn_signal_left_status = turn_signal_left_status;
    if (turn_signal_right_status != prev_turn_signal_right_status) prev_turn_signal_right_status = turn_signal_right_status;
  }

  void saveBlindSpotColors(bool isLeft) {
    if (isLeft) {
      for (uint16_t i = leftBlindSpotStart; i <= leftBlindSpotStop; i++) {
        leftBlindSpotColors[i - leftBlindSpotStart] = strip.getPixelColor(i);
      }
    } else {
      for (uint16_t i = rightBlindSpotStart; i <= rightBlindSpotStop; i++) {
        rightBlindSpotColors[i - rightBlindSpotStart] = strip.getPixelColor(i);
      }
    }
  }

  void setBlindSpotColor(bool isLeft, CRGB color) {
    if (isLeft) {
      for (uint16_t i = leftBlindSpotStart; i <= leftBlindSpotStop; i++) {
        strip.setPixelColor(i, color);
      }
    } else {
      for (uint16_t i = rightBlindSpotStart; i <= rightBlindSpotStop; i++) {
        strip.setPixelColor(i, color);
      }
    }
  }

  void restoreBlindSpotColors(bool isLeft) {
    if (isLeft) {
      for (uint16_t i = leftBlindSpotStart; i <= leftBlindSpotStop; i++) {
        strip.setPixelColor(i, leftBlindSpotColors[i - leftBlindSpotStart]);
      }
    } else {
      for (uint16_t i = rightBlindSpotStart; i <= rightBlindSpotStop; i++) {
        strip.setPixelColor(i, rightBlindSpotColors[i - rightBlindSpotStart]);
      }
    }
  }

  void saveBrakeLightColors() {
    for (uint16_t i = brakeLightStart; i <= brakeLightStop; i++) {
      brakeLightColors[i - brakeLightStart] = strip.getPixelColor(i);
    }
  }

  void setBrakeLightColor(CRGB color) {
    for (uint16_t i = brakeLightStart; i <= brakeLightStop; i++) {
      strip.setPixelColor(i, color);
    }
  }

  void restoreBrakeLightColors() {
    for (uint16_t i = brakeLightStart; i <= brakeLightStop; i++) {
      strip.setPixelColor(i, brakeLightColors[i - brakeLightStart]);
    }
  }

  void saveCollisionWarningColors() {
    for (uint16_t i = collisionWarningStart; i <= collisionWarningStop; i++) {
      collisionWarningColors[i - collisionWarningStart] = strip.getPixelColor(i);
    }
  }

  void setCollisionWarningColor(CRGB color) {
    for (uint16_t i = collisionWarningStart; i <= collisionWarningStop; i++) {
      strip.setPixelColor(i, color);
    }
  }

  void restoreCollisionWarningColors() {
    for (uint16_t i = collisionWarningStart; i <= collisionWarningStop; i++) {
      strip.setPixelColor(i, collisionWarningColors[i - collisionWarningStart]);
    }
  }

  void handleBlindSpots() {
    bool leftTurnSignal = (turn_signal_left_status == 1);    // LIGHT_ON
    bool rightTurnSignal = (turn_signal_right_status == 1);  // LIGHT_ON
    unsigned long currentTime = millis();

    // Left Blind Spot
    bool leftWarning = (blind_spot_left == 1 || blind_spot_left == 2);
    if (leftWarning && !blindSpotActiveLeft) {
      saveBlindSpotColors(true);
      blindSpotActiveLeft = true;
      SERIAL_PRINTLN("Left Blind Spot ON: " + String(leftTurnSignal ? "Red" : "RGB(" + String(blindSpotRGB.r) + "," + String(blindSpotRGB.g) + "," + String(blindSpotRGB.b) + ")"));
    } else if (!leftWarning && blindSpotActiveLeft) {
      restoreBlindSpotColors(true);
      blindSpotActiveLeft = false;
      SERIAL_PRINTLN("Left Blind Spot OFF");
    }

    // Right Blind Spot
    bool rightWarning = (blind_spot_right == 1 || blind_spot_right == 2);
    if (rightWarning && !blindSpotActiveRight) {
      saveBlindSpotColors(false);
      blindSpotActiveRight = true;
      SERIAL_PRINTLN("Right Blind Spot ON: " + String(rightTurnSignal ? "Red" : "RGB(" + String(blindSpotRGB.r) + "," + String(blindSpotRGB.g) + "," + String(blindSpotRGB.b) + ")"));
    } else if (!rightWarning && blindSpotActiveRight) {
      restoreBlindSpotColors(false);
      blindSpotActiveRight = false;
      SERIAL_PRINTLN("Right Blind Spot OFF");
    }

    // Handle turn signal debounce for left
    if (leftTurnSignal && blindSpotActiveLeft) {
      leftTurnSignalRedActive = true;
      leftTurnSignalOffTime = 0;
    } else if (!leftTurnSignal && leftTurnSignalRedActive) {
      if (leftTurnSignalOffTime == 0) {
        leftTurnSignalOffTime = currentTime;
      } else if (currentTime - leftTurnSignalOffTime >= TURN_SIGNAL_DELAY) {
        leftTurnSignalRedActive = false;
        SERIAL_PRINTLN("Left Turn Signal Red OFF after delay");
      }
    }

    // Handle turn signal debounce for right
    if (rightTurnSignal && blindSpotActiveRight) {
      rightTurnSignalRedActive = true;
      rightTurnSignalOffTime = 0;
    } else if (!rightTurnSignal && rightTurnSignalRedActive) {
      if (rightTurnSignalOffTime == 0) {
        rightTurnSignalOffTime = currentTime;
      } else if (currentTime - rightTurnSignalOffTime >= TURN_SIGNAL_DELAY) {
        rightTurnSignalRedActive = false;
        SERIAL_PRINTLN("Right Turn Signal Red OFF after delay");
      }
    }
  }

  void handleBrakeLight() {
    bool brakeOn = (brake_status == 1);
    if (brakeOn && !brakeLightActive) {
      saveBrakeLightColors();
      brakeLightActive = true;
      SERIAL_PRINTLN("Brake Light ON: Red - Overlay Enabled");
    } else if (!brakeOn && brakeLightActive) {
      restoreBrakeLightColors();
      brakeLightActive = false;
      SERIAL_PRINTLN("Brake Light OFF");
    }
  }

  void handleCollisionWarning() {
      // Does not work now, Signal does not properly send
    bool isWarningActive = (forward_collision != 0 && forward_collision != 3); // Ignore NONE and SNA ( signal not avaliable)
    if (isWarningActive && !collisionWarningActive) {
      saveCollisionWarningColors();
      collisionWarningActive = true;
      SERIAL_PRINTLN("Collision Warning ON: Red");
    } else if (!isWarningActive && collisionWarningActive) {
      restoreCollisionWarningColors();
      collisionWarningActive = false;
      SERIAL_PRINTLN("Collision Warning OFF");
    }
  }

  // Determine if footwell lights should be active based on is_sun_up and display_brightness_level
  bool isFootwellActive() {
    if (is_sun_up == 0) { // Dark outside, footwell lights always on
      SERIAL_PRINTLN("Footwell lights ON: Dark outside (is_sun_up = 0)");
      return true;
    } else { // Sun is up, check display brightness to make sure that they are not on when too bright out
      bool footwellOn = (display_brightness_level < 80.0);
      SERIAL_PRINTLN("Footwell lights " + String(footwellOn ? "ON" : "OFF") + ": Sun up, brightness = " + String(display_brightness_level) + "%");
      return footwellOn;
    }
  }
  
  void handleOverlayDraw() override {
    // Get the main segment and brightness settings
    Segment& mainSegment = strip.getSegment(strip.getMainSegmentId());
    uint8_t segmentBrightness = mainSegment.opacity;
    uint8_t globalBrightness = strip.getBrightness();
    uint8_t effectiveBrightness = (uint8_t)((uint16_t)segmentBrightness * globalBrightness / 255);

    // Handle footwell lights (segment 2)
    Segment& footwellSegment = strip.getSegment(2); // Footwell lights are segment 2
    if (isFootwellActive()) {
      // Footwell lights should be on, ensure segment is active
      if (!footwellSegment.isActive()) {
        footwellSegment.setOption(SEG_OPTION_ON, true); // Turn segment on
        SERIAL_PRINTLN("Footwell segment activated");
      }
    } else {
      // Footwell lights should be off, deactivate segment
      if (footwellSegment.isActive()) {
        footwellSegment.setOption(SEG_OPTION_ON, false); // Turn segment off
        SERIAL_PRINTLN("Footwell segment deactivated");
      }
    }

    // Brake Light
    if (brakeLightActive) {
      CRGB scaledRed = CRGB::Red;
      scaledRed.fadeToBlackBy(255 - effectiveBrightness);
      setBrakeLightColor(scaledRed);
      #if DEBUG
        SERIAL_PRINTLN("Brake Light Color: R=" + String(scaledRed.r) + ", G=" + String(scaledRed.g) + ", B=" + String(scaledRed.b));
      #endif
    }

    // Blind Spots
    if (UseBlindSpot) {
      if (blindSpotActiveLeft) {
        CRGB scaledColor = (leftTurnSignalRedActive) ? CRGB::Red : CRGB(blindSpotRGB.r, blindSpotRGB.g, blindSpotRGB.b);
        scaledColor.fadeToBlackBy(255 - effectiveBrightness);
        setBlindSpotColor(true, scaledColor);
        #if DEBUG
          SERIAL_PRINTLN("Left Blind Spot Color: R=" + String(scaledColor.r) + ", G=" + String(scaledColor.g) + ", B=" + String(scaledColor.b));
        #endif
      }
      if (blindSpotActiveRight) {
        CRGB scaledColor = (rightTurnSignalRedActive) ? CRGB::Red : CRGB(blindSpotRGB.r, blindSpotRGB.g, blindSpotRGB.b);
        scaledColor.fadeToBlackBy(255 - effectiveBrightness);
        setBlindSpotColor(false, scaledColor);
        #if DEBUG
          SERIAL_PRINTLN("Right Blind Spot Color: R=" + String(scaledColor.r) + ", G=" + String(scaledColor.g) + ", B=" + String(scaledColor.b));
        #endif
      }
    }

    // Collision Warning
    if (UseCollisionWarning && collisionWarningActive) {
      CRGB scaledColor = CRGB::Red;
      scaledColor.fadeToBlackBy(255 - effectiveBrightness);
      setCollisionWarningColor(scaledColor);
      #if DEBUG
        SERIAL_PRINTLN("Collision Warning Color: R=" + String(scaledRed.r) + ", G=" + String(scaledRed.g) + ", B=" + String(scaledRed.b));
      #endif
    }

    // Ensure WLED respects the overlay colors
    colorUpdated(CALL_MODE_NOTIFICATION);
  }

  void loop() override {
    if (enabled) {
      if (UseESPNow) {
        EspNowHandling();
        if (UseBlindSpot) handleBlindSpots();
        if (UseBrakeLight) handleBrakeLight();
        if (UseCollisionWarning) handleCollisionWarning();
        // Update footwell lights state
        isFootwellActive(); // Call to update status and log
      }
    } else {
      Status = "Not Enabled or Init";
    }
  }

  void addToJsonState(JsonObject& root) override {
    if (!initDone) return;
    JsonObject um = root[FPSTR(_name)];
    if (um.isNull()) um = root.createNestedObject(FPSTR(_name));
    um["on"] = enabled;
    um[FPSTR(_UseESPNow)] = UseESPNow;
    um[FPSTR(_UseBlindSpot)] = UseBlindSpot;
    um[FPSTR(_UseBrakeLight)] = UseBrakeLight;
    um[FPSTR(_UseVehicleSWC)] = UseVehicleSWC;
    um[FPSTR(_UseFrontOccupancy)] = UseFrontOccupancy;
    um[FPSTR(_UseRearOccupancy)] = UseRearOccupancy;
    um[FPSTR(_UseCollisionWarning)] = UseCollisionWarning;
    um[FPSTR(_UseCarWashMode)] = UseCarWashMode; 
    um[FPSTR(_brakeLightStart)] = brakeLightStart;
    um[FPSTR(_brakeLightStop)] = brakeLightStop;
    um[FPSTR(_leftBlindSpotStart)] = leftBlindSpotStart;
    um[FPSTR(_leftBlindSpotStop)] = leftBlindSpotStop;
    um[FPSTR(_rightBlindSpotStart)] = rightBlindSpotStart;
    um[FPSTR(_rightBlindSpotStop)] = rightBlindSpotStop;
    um[FPSTR(_leftBlindSpotStart)] = leftBlindSpotStart;
    um[FPSTR(_leftBlindSpotStop)] = leftBlindSpotStop;
    um[FPSTR(_rightBlindSpotStart)] = rightBlindSpotStart;
    um[FPSTR(_rightBlindSpotStop)] = rightBlindSpotStop;
    um[FPSTR(_collisionWarningStart)] = collisionWarningStart;
    um[FPSTR(_collisionWarningStop)] = collisionWarningStop;
    um[FPSTR(_blindSpotRGB)] = String(blindSpotRGB.r) + "," + String(blindSpotRGB.g) + "," + String(blindSpotRGB.b);
  }

  void addToJsonInfo(JsonObject& root) override {
    JsonObject U = root["u"];
    if (U.isNull()) U = root.createNestedObject("u");
    JsonArray infoArr = U.createNestedArray(FPSTR(_name));

    String uiDomString = F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
    uiDomString += FPSTR(_name);
    uiDomString += F(":{");
    uiDomString += FPSTR(_enabled);
    uiDomString += enabled ? F(":false}});\">") : F(":true}});\">");
    uiDomString += F("<i class=\"icons");
    uiDomString += enabled ? F(" on") : F(" off");
    uiDomString += F("\"></i>");
    uiDomString += F("</button>");
    infoArr.add(uiDomString);

    JsonArray statusReadArr = U.createNestedArray(F("Status"));
    statusReadArr.add(Status);

    if (UseESPNow) {
      JsonArray BrightArr = U.createNestedArray(F("Brightness:"));
      BrightArr.add(String(display_brightness_level) + "%");
      JsonArray OccuArr = U.createNestedArray(F("Rear Occupancy:"));
      OccuArr.add(String(rear_occupancy ? "Yes" : "No"));
      JsonArray CarWashArr = U.createNestedArray(F("Car Wash Mode:")); 
      CarWashArr.add(String(car_wash_mode_request ? "Yes" : "No")); 
    }
    if (UseBlindSpot) {
      JsonArray LeftBlindArr = U.createNestedArray(F("Left Blind:"));
      LeftBlindArr.add(String(blind_spot_left));
      JsonArray RightBlindArr = U.createNestedArray(F("Right Blind:"));
      RightBlindArr.add(String(blind_spot_right));
      JsonArray ColorArr = U.createNestedArray(F("Blind Spot RGB:"));
      ColorArr.add(String(blindSpotRGB.r) + "," + String(blindSpotRGB.g) + "," + String(blindSpotRGB.b));
    }
    
    JsonArray SunArr = U.createNestedArray(F("Is Sun Up:"));
    SunArr.add(String(is_sun_up ? "Yes" : "No"));

    if (UseNightBrightnessCap) {
      JsonArray CapArr = U.createNestedArray(F("Night Brightness Cap:"));
      CapArr.add(String(nightBrightnessCapPct, 0) + "%" + (is_sun_up == 0 ? " [ACTIVE]" : " [inactive]"));
    }

    JsonArray SpdArr = U.createNestedArray(F("Speed:"));
    SpdArr.add(String(vehicle_speed_mph) + " mph");
  }

  void readFromJsonState(JsonObject& root) override {
    if (!initDone) return;
    SERIAL_PRINTLN("readFromJsonState called");
    bool prevEnabled = enabled;
    JsonObject um = root[FPSTR(_name)];
    if (!um.isNull()) {
      SERIAL_PRINTLN("Found Ambience object in JSON");
      if (um[FPSTR(_enabled)].is<bool>()) {
        enabled = um[FPSTR(_enabled)].as<bool>();
        if (prevEnabled != enabled) onUpdateBegin(!enabled);
        SERIAL_PRINTLN("Enabled: " + String(enabled ? "ON" : "OFF"));
      }
      if (um[FPSTR(_UseESPNow)].is<bool>()) UseESPNow = um[FPSTR(_UseESPNow)].as<bool>();
      if (um[FPSTR(_UseBlindSpot)].is<bool>()) UseBlindSpot = um[FPSTR(_UseBlindSpot)].as<bool>();
      if (um[FPSTR(_UseBrakeLight)].is<bool>()) UseBrakeLight = um[FPSTR(_UseBrakeLight)].as<bool>();
      if (um[FPSTR(_UseVehicleSWC)].is<bool>()) UseVehicleSWC = um[FPSTR(_UseVehicleSWC)].as<bool>();
      if (um[FPSTR(_UseFrontOccupancy)].is<bool>()) UseFrontOccupancy = um[FPSTR(_UseFrontOccupancy)].as<bool>();
      if (um[FPSTR(_UseRearOccupancy)].is<bool>()) UseRearOccupancy = um[FPSTR(_UseRearOccupancy)].as<bool>();
      if (um[FPSTR(_UseCollisionWarning)].is<bool>()) UseCollisionWarning = um[FPSTR(_UseCollisionWarning)].as<bool>();
      if (um[FPSTR(_UseCarWashMode)].is<bool>()) UseCarWashMode = um[FPSTR(_UseCarWashMode)].as<bool>(); 
      if (um[FPSTR(_brakeLightStart)].is<uint16_t>()) brakeLightStart = um[FPSTR(_brakeLightStart)].as<uint16_t>();
      if (um[FPSTR(_brakeLightStop)].is<uint16_t>()) brakeLightStop = um[FPSTR(_brakeLightStop)].as<uint16_t>();
      if (um[FPSTR(_leftBlindSpotStart)].is<uint16_t>()) leftBlindSpotStart = um[FPSTR(_leftBlindSpotStart)].as<uint16_t>();
      if (um[FPSTR(_leftBlindSpotStop)].is<uint16_t>()) leftBlindSpotStop = um[FPSTR(_leftBlindSpotStop)].as<uint16_t>();
      if (um[FPSTR(_rightBlindSpotStart)].is<uint16_t>()) rightBlindSpotStart = um[FPSTR(_rightBlindSpotStart)].as<uint16_t>();
      if (um[FPSTR(_rightBlindSpotStop)].is<uint16_t>()) rightBlindSpotStop = um[FPSTR(_rightBlindSpotStop)].as<uint16_t>();
      if (um[FPSTR(_collisionWarningStart)].is<uint16_t>()) collisionWarningStart = um[FPSTR(_collisionWarningStart)].as<uint16_t>();
      if (um[FPSTR(_collisionWarningStop)].is<uint16_t>()) collisionWarningStop = um[FPSTR(_collisionWarningStop)].as<uint16_t>();
      if (um[FPSTR(_blindSpotRGB)].is<String>()) {
        String rgbStr = um[FPSTR(_blindSpotRGB)].as<String>();
        parseRGBString(rgbStr);
      }
    } else {
      SERIAL_PRINTLN("Ambience object not found in JSON");
    }
  }

  void parseRGBString(String rgbStr) {
    // Expected format: "R,G,B" (e.g., "255,140,0")
    int firstComma = rgbStr.indexOf(',');
    int secondComma = rgbStr.indexOf(',', firstComma + 1);

    if (firstComma == -1 || secondComma == -1) {
      SERIAL_PRINTLN("Invalid RGB format: " + rgbStr + ", keeping current values");
      return;
    }

    String rStr = rgbStr.substring(0, firstComma);
    String gStr = rgbStr.substring(firstComma + 1, secondComma);
    String bStr = rgbStr.substring(secondComma + 1);

    int r = rStr.toInt();
    int g = gStr.toInt();
    int b = bStr.toInt();

    // Validate RGB values
    if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
      blindSpotRGB.r = r;
      blindSpotRGB.g = g;
      blindSpotRGB.b = b;
      SERIAL_PRINTLN("Updated blindSpotRGB to: R=" + String(r) + ", G=" + String(g) + ", B=" + String(b));
    } else {
      SERIAL_PRINTLN("Invalid RGB values: R=" + String(r) + ", G=" + String(g) + ", B=" + String(b) + ", keeping current values");
    }
  }

  void addToConfig(JsonObject& root) override {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top[FPSTR(_enabled)] = enabled;

    JsonObject ledConfig = top.createNestedObject("LED Ranges");
    ledConfig[FPSTR(_brakeLightStart)] = brakeLightStart;
    ledConfig[FPSTR(_brakeLightStop)] = brakeLightStop;
    ledConfig[FPSTR(_leftBlindSpotStart)] = leftBlindSpotStart;
    ledConfig[FPSTR(_leftBlindSpotStop)] = leftBlindSpotStop;
    ledConfig[FPSTR(_rightBlindSpotStart)] = rightBlindSpotStart;
    ledConfig[FPSTR(_rightBlindSpotStop)] = rightBlindSpotStop;
    ledConfig[FPSTR(_collisionWarningStart)] = collisionWarningStart;
    ledConfig[FPSTR(_collisionWarningStop)] = collisionWarningStop;

    JsonObject colorConfig = top.createNestedObject("Colors");
    colorConfig[FPSTR(_blindSpotRGB)] = String(blindSpotRGB.r) + "," + String(blindSpotRGB.g) + "," + String(blindSpotRGB.b);

    JsonObject presetConfig = top.createNestedObject("Presets");
    presetConfig[FPSTR(_PRST_1_10)] = PRST_1_10;
    presetConfig[FPSTR(_PRST_11_20)] = PRST_11_20;
    presetConfig[FPSTR(_PRST_21_30)] = PRST_21_30;
    presetConfig[FPSTR(_PRST_31_40)] = PRST_31_40;
    presetConfig[FPSTR(_PRST_41_50)] = PRST_41_50;
    presetConfig[FPSTR(_PRST_51_60)] = PRST_51_60;
    presetConfig[FPSTR(_PRST_61_70)] = PRST_61_70;
    presetConfig[FPSTR(_PRST_71_80)] = PRST_71_80;
    presetConfig[FPSTR(_PRST_81_90)] = PRST_81_90;
    presetConfig[FPSTR(_PRST_91_100)] = PRST_91_100;
    presetConfig[FPSTR(_PRST_SWC_LEFT_HELD)] = PRST_SWC_LEFT_HELD;
    presetConfig[FPSTR(_PRST_SWC_LEFT_DOUBLE_PRESS)] = PRST_SWC_LEFT_DOUBLE_PRESS;
    presetConfig[FPSTR(_PRST_SWC_LEFT_TILT_LEFT)] = PRST_SWC_LEFT_TILT_LEFT;
    presetConfig[FPSTR(_PRST_SWC_LEFT_TILT_RIGHT)] = PRST_SWC_LEFT_TILT_RIGHT;
    presetConfig[FPSTR(_PRST_SWC_RIGHT_HELD)] = PRST_SWC_RIGHT_HELD;
    presetConfig[FPSTR(_PRST_SWC_RIGHT_DOUBLE_PRESS)] = PRST_SWC_RIGHT_DOUBLE_PRESS;
    presetConfig[FPSTR(_PRST_SWC_RIGHT_TILT_LEFT)] = PRST_SWC_RIGHT_TILT_LEFT;
    presetConfig[FPSTR(_PRST_SWC_RIGHT_TILT_RIGHT)] = PRST_SWC_RIGHT_TILT_RIGHT;
    presetConfig[FPSTR(_PRST_FRONT_OCCUPANCY_ON)] = PRST_FRONT_OCCUPANCY_ON;
    presetConfig[FPSTR(_PRST_FRONT_OCCUPANCY_OFF)] = PRST_FRONT_OCCUPANCY_OFF;
    presetConfig[FPSTR(_PRST_REAR_OCCUPANCY_ON)] = PRST_REAR_OCCUPANCY_ON;
    presetConfig[FPSTR(_PRST_REAR_OCCUPANCY_OFF)] = PRST_REAR_OCCUPANCY_OFF;
    presetConfig[FPSTR(_PRST_CAR_WASH_MODE_ON)] = PRST_CAR_WASH_MODE_ON; 
    presetConfig[FPSTR(_PRST_CAR_WASH_MODE_OFF)] = PRST_CAR_WASH_MODE_OFF; 
    
    JsonObject SwitchConfig = top.createNestedObject("Logic Select");
    SwitchConfig[FPSTR(_UseESPNow)] = UseESPNow;
    SwitchConfig[FPSTR(_UseBlindSpot)] = UseBlindSpot;
    SwitchConfig[FPSTR(_UseBrakeLight)] = UseBrakeLight;
    SwitchConfig[FPSTR(_UseVehicleSWC)] = UseVehicleSWC;
    SwitchConfig[FPSTR(_UseFrontOccupancy)] = UseFrontOccupancy;
    SwitchConfig[FPSTR(_UseRearOccupancy)] = UseRearOccupancy;
    SwitchConfig[FPSTR(_UseCollisionWarning)] = UseCollisionWarning;
    SwitchConfig[FPSTR(_UseCarWashMode)] = UseCarWashMode;
    SwitchConfig[FPSTR(_UseNightBrightnessCap)] = UseNightBrightnessCap;
    SwitchConfig[FPSTR(_nightBrightnessCapPct)] = nightBrightnessCapPct;

    #if DEBUG
      SERIAL_PRINTLN("addToConfig JSON:");
      serializeJson(root, Serial);
      SERIAL_PRINTLN();
    #endif
  }

  bool readFromConfig(JsonObject& root) override {
    JsonObject top = root[FPSTR(_name)];
    bool configComplete = !top.isNull();

    configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
    
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_brakeLightStart)], brakeLightStart);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_brakeLightStop)], brakeLightStop);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_leftBlindSpotStart)], leftBlindSpotStart);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_leftBlindSpotStop)], leftBlindSpotStop);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_rightBlindSpotStart)], rightBlindSpotStart);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_rightBlindSpotStop)], rightBlindSpotStop);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_collisionWarningStart)], collisionWarningStart);
    configComplete &= getJsonValue(top["LED Ranges"][FPSTR(_collisionWarningStop)], collisionWarningStop);

    String rgbStr;
    if (getJsonValue(top["Colors"][FPSTR(_blindSpotRGB)], rgbStr)) {
      parseRGBString(rgbStr);
    }

    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_1_10)], PRST_1_10);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_11_20)], PRST_11_20);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_21_30)], PRST_21_30);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_31_40)], PRST_31_40);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_41_50)], PRST_41_50);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_51_60)], PRST_51_60);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_61_70)], PRST_61_70);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_71_80)], PRST_71_80);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_81_90)], PRST_81_90);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_91_100)], PRST_91_100);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_LEFT_HELD)], PRST_SWC_LEFT_HELD);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_LEFT_DOUBLE_PRESS)], PRST_SWC_LEFT_DOUBLE_PRESS);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_LEFT_TILT_LEFT)], PRST_SWC_LEFT_TILT_LEFT);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_LEFT_TILT_RIGHT)], PRST_SWC_LEFT_TILT_RIGHT);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_RIGHT_HELD)], PRST_SWC_RIGHT_HELD);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_RIGHT_DOUBLE_PRESS)], PRST_SWC_RIGHT_DOUBLE_PRESS);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_RIGHT_TILT_LEFT)], PRST_SWC_RIGHT_TILT_LEFT);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_SWC_RIGHT_TILT_RIGHT)], PRST_SWC_RIGHT_TILT_RIGHT);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_FRONT_OCCUPANCY_ON)], PRST_FRONT_OCCUPANCY_ON);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_FRONT_OCCUPANCY_OFF)], PRST_FRONT_OCCUPANCY_OFF);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_REAR_OCCUPANCY_ON)], PRST_REAR_OCCUPANCY_ON);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_REAR_OCCUPANCY_OFF)], PRST_REAR_OCCUPANCY_OFF);
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_CAR_WASH_MODE_ON)], PRST_CAR_WASH_MODE_ON); 
    configComplete &= getJsonValue(top["Presets"][FPSTR(_PRST_CAR_WASH_MODE_OFF)], PRST_CAR_WASH_MODE_OFF); 
    
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseESPNow)], UseESPNow);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseBlindSpot)], UseBlindSpot);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseBrakeLight)], UseBrakeLight);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseVehicleSWC)], UseVehicleSWC);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseFrontOccupancy)], UseFrontOccupancy);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseRearOccupancy)], UseRearOccupancy);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseCollisionWarning)], UseCollisionWarning);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseCarWashMode)], UseCarWashMode);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_UseNightBrightnessCap)], UseNightBrightnessCap);
    configComplete &= getJsonValue(top["Logic Select"][FPSTR(_nightBrightnessCapPct)], nightBrightnessCapPct);
    
    return configComplete;
  }


  uint16_t getId() override {
    return USERMOD_ID_UNSPECIFIED;  // modern v2 – no custom ID needed
  }
};


// All your constexpr definitions at the bottom – unchanged
constexpr char AmbienceV1::_name[] PROGMEM;
constexpr char AmbienceV1::_enabled[] PROGMEM;
constexpr char AmbienceV1::_UseESPNow[] PROGMEM;
constexpr char AmbienceV1::_UseBlindSpot[] PROGMEM;
constexpr char AmbienceV1::_UseBrakeLight[] PROGMEM;
constexpr char AmbienceV1::_UseVehicleSWC[] PROGMEM;
constexpr char AmbienceV1::_UseFrontOccupancy[] PROGMEM;
constexpr char AmbienceV1::_UseRearOccupancy[] PROGMEM;
constexpr char AmbienceV1::_UseCollisionWarning[] PROGMEM;
constexpr char AmbienceV1::_UseCarWashMode[] PROGMEM;
constexpr char AmbienceV1::_UseNightBrightnessCap[] PROGMEM;
constexpr char AmbienceV1::_nightBrightnessCapPct[] PROGMEM;
constexpr char AmbienceV1::_brakeLightStart[] PROGMEM;
constexpr char AmbienceV1::_brakeLightStop[] PROGMEM;
constexpr char AmbienceV1::_leftBlindSpotStart[] PROGMEM;
constexpr char AmbienceV1::_leftBlindSpotStop[] PROGMEM;
constexpr char AmbienceV1::_rightBlindSpotStart[] PROGMEM;
constexpr char AmbienceV1::_rightBlindSpotStop[] PROGMEM;
constexpr char AmbienceV1::_collisionWarningStart[] PROGMEM;
constexpr char AmbienceV1::_collisionWarningStop[] PROGMEM;
constexpr char AmbienceV1::_blindSpotRGB[] PROGMEM;
constexpr char AmbienceV1::_PRST_1_10[] PROGMEM;
constexpr char AmbienceV1::_PRST_11_20[] PROGMEM;
constexpr char AmbienceV1::_PRST_21_30[] PROGMEM;
constexpr char AmbienceV1::_PRST_31_40[] PROGMEM;
constexpr char AmbienceV1::_PRST_41_50[] PROGMEM;
constexpr char AmbienceV1::_PRST_51_60[] PROGMEM;
constexpr char AmbienceV1::_PRST_61_70[] PROGMEM;
constexpr char AmbienceV1::_PRST_71_80[] PROGMEM;
constexpr char AmbienceV1::_PRST_81_90[] PROGMEM;
constexpr char AmbienceV1::_PRST_91_100[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_LEFT_HELD[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_LEFT_DOUBLE_PRESS[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_LEFT_TILT_LEFT[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_LEFT_TILT_RIGHT[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_RIGHT_HELD[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_RIGHT_DOUBLE_PRESS[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_RIGHT_TILT_LEFT[] PROGMEM;
constexpr char AmbienceV1::_PRST_SWC_RIGHT_TILT_RIGHT[] PROGMEM;
constexpr char AmbienceV1::_PRST_FRONT_OCCUPANCY_ON[] PROGMEM;
constexpr char AmbienceV1::_PRST_FRONT_OCCUPANCY_OFF[] PROGMEM;
constexpr char AmbienceV1::_PRST_REAR_OCCUPANCY_ON[] PROGMEM;
constexpr char AmbienceV1::_PRST_REAR_OCCUPANCY_OFF[] PROGMEM;
constexpr char AmbienceV1::_PRST_CAR_WASH_MODE_ON[] PROGMEM; 
constexpr char AmbienceV1::_PRST_CAR_WASH_MODE_OFF[] PROGMEM;
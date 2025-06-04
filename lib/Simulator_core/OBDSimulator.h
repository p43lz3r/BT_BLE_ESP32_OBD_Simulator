#ifndef OBD_SIMULATOR_H
#define OBD_SIMULATOR_H

#include <Arduino.h>
#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE UUIDs (Nordic UART Service compatible)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Simulation data structure
struct SimulatedData {
  float rpm = 800.0;
  float speed = 0.0;
  float coolantTemp = 90.0;
  float oilTemp = 85.0;
  float fuelLevel = 75.0;
  float throttlePos = 0.0;
  float boostPressure = 0.0;
  float airflowRate = 15.0;
  int engineLoad = 25;
  bool engineRunning = true;
};

// ELM327 state structure
struct ELMState {
  bool echoOn = true;
  bool headersOn = false;
  bool spacesOn = true;
  bool lineFeedsOn = true;
  String protocol = "6";
  bool adaptiveTiming = true;
  int timeout = 200;
};

// Main OBD Simulator class
class OBDSimulator {
public:
  // Constructor
  OBDSimulator();
  
  // Initialization
  void begin();
  void setupClassicBT();
  void setupBLE();
  
  // Main loop processing
  void loop();
  
  // Data management
  void updateSimulatedData();
  void initializeSimulatedData();
  
  // Command processing
  String processOBDCommand(String cmd, String interface);
  String processATCommand(String cmd);
  String processOBDPID(String mode, String pid);
  
  // Response handling
  void sendClassicResponse(String cmd, String response);
  void sendBLEResponse(String response);
  
  // Utility functions
  String formatResponse(String response);
  String formatHex(int value);
  
  // Status getters
  bool isClassicConnected() const { return classicConnected; }
  bool isBLEConnected() const { return bleConnected; }
  SimulatedData getCurrentData() const { return simData; }
  
  // Configuration
  void setDebugMode(bool enabled) { debugMode = enabled; }
  void setDeviceName(String classic, String ble);
  
private:
  // Connection state
  bool classicConnected = false;
  bool bleConnected = false;
  bool deviceConnected = false;
  bool oldDeviceConnected = false;
  
  // Timing
  unsigned long lastDataUpdate = 0;
  unsigned long connectionTime = 0;
  unsigned long lastDebugOutput = 0;
  int commandCount = 0;
  
  // Data structures
  SimulatedData simData;
  ELMState elmState;
  String lastCommand = "";
  
  // Settings
  bool debugMode = true;
  String classicBTName = "OBD2_Simulator_Dual";
  String bleName = "OBD2_Simulator_BLE";
  
  // Bluetooth Classic
  BluetoothSerial* pSerialBT = nullptr;
  
  // BLE components
  BLEServer* pServer = nullptr;
  BLECharacteristic* pTxCharacteristic = nullptr;
  BLECharacteristic* pRxCharacteristic = nullptr;
  
  // Private methods
  void printSystemInfo();
  void printStatus();
  void resetELMState();
  
  // Friend classes for callbacks
  friend class MyServerCallbacks;
  friend class MyCallbacks;
  friend void btClassicCallbackFunction(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
};

// Global instance pointer for callbacks
extern OBDSimulator* g_simulator;

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
public:
  void onConnect(BLEServer* pServer) override;
  void onDisconnect(BLEServer* pServer) override;
};

// BLE Characteristic Callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *pCharacteristic) override;
};

// Classic Bluetooth callback function
void btClassicCallbackFunction(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

#endif // OBD_SIMULATOR_H
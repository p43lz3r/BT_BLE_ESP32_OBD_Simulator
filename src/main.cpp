/*
 * ESP32 Dual-Mode OBD2 Simulator - Main Application
 * Modular version with clean separation of concerns
 * 
 * Features:
 * - Bluetooth Classic support (smartphones, ELMduino, Torque Pro)
 * - BLE support (modern devices, ESP32-S3 clients)
 * - Full ELM327 protocol compatibility
 * - Realistic engine simulation
 * - Professional debugging output
 */

#include <Arduino.h>
#include "OBDSimulator.h"

// Create simulator instance
OBDSimulator simulator;

void setup() {
  Serial.begin(115200);
  delay(2000); // Allow serial to initialize
  
  // Configure simulator
  simulator.setDebugMode(true);
  simulator.setDeviceName("OBD2_Simulator_Dual", "OBD2_Simulator_BLE");
  
  // Initialize the simulator
  simulator.begin();
}

void loop() {
  // Run the simulator
  simulator.loop();
  
  // Small delay for stability
  delay(10);
}
#include "OBDSimulator.h"

// Global instance pointer
OBDSimulator* g_simulator = nullptr;

// Constructor
OBDSimulator::OBDSimulator() {
  g_simulator = this;
  pSerialBT = new BluetoothSerial();
}

// Main initialization
void OBDSimulator::begin() {
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║        ESP32 Dual-Mode OBD2 Simulator         ║");
  Serial.println("║      Bluetooth Classic + BLE Support          ║");
  Serial.println("║            ELM327 Compatible                  ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
  
  printSystemInfo();
  initializeSimulatedData();
  setupClassicBT();
  setupBLE();
  
  Serial.println();
  Serial.println("🎉 DUAL-MODE SIMULATOR READY!");
  Serial.println("📱 Classic BT: " + classicBTName);
  Serial.println("📱 BLE: " + bleName);
  Serial.println("⏳ Waiting for connections...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println();
}

void OBDSimulator::setupClassicBT() {
  Serial.println("🔵 Starting Bluetooth Classic...");
  pSerialBT->begin(classicBTName);
  pSerialBT->register_callback(btClassicCallbackFunction);
  Serial.println("✅ Bluetooth Classic ready: " + classicBTName);
}

void OBDSimulator::setupBLE() {
  Serial.println("🔵 Starting Bluetooth Low Energy...");
  
  // Create BLE Device
  BLEDevice::init(bleName);
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service (Nordic UART Service compatible)
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX Characteristic (for sending data to client)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create RX Characteristic (for receiving data from client)
  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  
  Serial.println("✅ BLE ready: " + bleName);
}

void OBDSimulator::loop() {
  // Update simulated data
  updateSimulatedData();
  
  // Handle Classic Bluetooth commands
  if (classicConnected && pSerialBT->available()) {
    String command = pSerialBT->readStringUntil('\r');
    command.trim();
    
    if (command.length() > 0) {
      commandCount++;
      unsigned long timeSinceConnection = millis() - connectionTime;
      
      if (debugMode) {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("📨 CLASSIC BT COMMAND #" + String(commandCount));
        Serial.println("⏰ Time: +" + String(timeSinceConnection) + " ms");
        Serial.println("📝 Raw: '" + command + "'");
      }
      
      String response = processOBDCommand(command, "Classic");
      sendClassicResponse(command, response);
      
      if (debugMode) {
        Serial.println("🔄 Classic Response: '" + response + "'");
        Serial.println("✅ Classic Response sent!");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      }
    }
  }
  
  // Handle BLE connection status changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give time for client to disconnect
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Periodic status output
  if (debugMode && (millis() - lastDebugOutput > 5000)) {
    printStatus();
    lastDebugOutput = millis();
  }
}

void OBDSimulator::initializeSimulatedData() {
  // Randomize initial values for realistic simulation
  simData.rpm = random(750, 850);
  simData.coolantTemp = random(85, 95);
  simData.oilTemp = random(80, 90);
  simData.fuelLevel = random(60, 90);
  simData.engineLoad = random(20, 30);
  simData.airflowRate = random(12, 18);
  
  Serial.println("🔧 Initialized simulation data:");
  Serial.println("   🔄 RPM: " + String(simData.rpm));
  Serial.println("   🌡️  Coolant: " + String(simData.coolantTemp) + "°C");
  Serial.println("   🛢️  Oil: " + String(simData.oilTemp) + "°C");
  Serial.println("   ⛽ Fuel: " + String(simData.fuelLevel) + "%");
}

void OBDSimulator::updateSimulatedData() {
  if (millis() - lastDataUpdate > 100) { // Update every 100ms
    // Simulate realistic engine behavior
    static float targetRPM = simData.rpm;
    static unsigned long rpmChangeTime = 0;
    
    if (millis() - rpmChangeTime > random(3000, 8000)) {
      targetRPM = random(750, 4000);
      rpmChangeTime = millis();
    }
    
    // Smooth RPM changes
    if (simData.rpm < targetRPM) {
      simData.rpm += random(5, 25);
    } else if (simData.rpm > targetRPM) {
      simData.rpm -= random(5, 25);
    }
    simData.rpm = constrain(simData.rpm, 700, 6000);
    
    // Update other parameters based on RPM
    simData.speed = map(simData.rpm, 700, 6000, 0, 120) + random(-5, 5);
    simData.speed = constrain(simData.speed, 0, 150);
    
    simData.throttlePos = map(simData.rpm, 700, 6000, 0, 80) + random(-10, 10);
    simData.throttlePos = constrain(simData.throttlePos, 0, 100);
    
    simData.engineLoad = map(simData.rpm, 700, 6000, 15, 85) + random(-5, 5);
    simData.engineLoad = constrain(simData.engineLoad, 0, 100);
    
    simData.airflowRate = map(simData.rpm, 700, 6000, 8, 45) + random(-2, 2);
    simData.airflowRate = constrain(simData.airflowRate, 5, 50);
    
    // Temperature variations
    simData.coolantTemp += random(-2, 2) * 0.1;
    simData.coolantTemp = constrain(simData.coolantTemp, 80, 110);
    
    simData.oilTemp += random(-2, 2) * 0.1;
    simData.oilTemp = constrain(simData.oilTemp, 75, 130);
    
    // Boost pressure (turbo simulation)
    if (simData.rpm > 2000 && simData.throttlePos > 50) {
      simData.boostPressure += random(-3, 8);
      simData.boostPressure = constrain(simData.boostPressure, 0, 150);
    } else {
      simData.boostPressure = max(0.0f, simData.boostPressure - 5);
    }
    
    // Fuel consumption simulation
    if (simData.engineLoad > 60) {
      simData.fuelLevel -= 0.001;
    }
    simData.fuelLevel = constrain(simData.fuelLevel, 5, 100);
    
    lastDataUpdate = millis();
  }
}

String OBDSimulator::processOBDCommand(String cmd, String interface) {
  String originalCmd = cmd;
  cmd.toUpperCase();
  cmd.trim();
  
  // Store last command for debugging
  lastCommand = cmd;
  
  // Clean command (remove spaces and non-alphanumeric)
  String cleanCmd = "";
  for (int i = 0; i < cmd.length(); i++) {
    char c = cmd.charAt(i);
    if (isAlphaNumeric(c)) {
      cleanCmd += c;
    }
  }
  cmd = cleanCmd;
  
  if (debugMode) {
    Serial.println("🧹 Cleaned: '" + cmd + "'");
    Serial.println("🔗 Interface: " + interface);
  }
  
  // Process AT commands
  if (cmd.startsWith("AT")) {
    return processATCommand(cmd);
  }
  
  // Process OBD2 PIDs
  if (cmd.length() >= 4) {
    String mode = cmd.substring(0, 2);
    String pid = cmd.substring(2, 4);
    return processOBDPID(mode, pid);
  }
  
  return "?";
}

String OBDSimulator::processATCommand(String cmd) {
  if (cmd == "ATZ") {
    resetELMState();
    delay(1500); // ELM327 reset delay
    return "ELM327 v1.5";
  }
  else if (cmd == "ATE0") { elmState.echoOn = false; return "OK"; }
  else if (cmd == "ATE1") { elmState.echoOn = true; return "OK"; }
  else if (cmd == "ATL0") { elmState.lineFeedsOn = false; return "OK"; }
  else if (cmd == "ATL1") { elmState.lineFeedsOn = true; return "OK"; }
  else if (cmd == "ATS0") { elmState.spacesOn = false; return "OK"; }
  else if (cmd == "ATS1") { elmState.spacesOn = true; return "OK"; }
  else if (cmd == "ATH0") { elmState.headersOn = false; return "OK"; }
  else if (cmd == "ATH1") { elmState.headersOn = true; return "OK"; }
  else if (cmd.startsWith("ATSP")) {
    elmState.protocol = cmd.substring(4);
    if (elmState.protocol == "0") elmState.protocol = "6";
    return "OK";
  }
  else if (cmd.startsWith("ATST")) {
    elmState.timeout = cmd.substring(4).toInt();
    return "OK";
  }
  else if (cmd == "ATDP") { return "ISO 15765-4 (CAN 11/500)"; }
  else if (cmd == "ATDPN") { return elmState.protocol; }
  else if (cmd == "ATI") { return "ELM327 v1.5"; }
  else if (cmd == "ATRV") { return "12.6V"; }
  else if (cmd == "ATAL") { elmState.adaptiveTiming = true; return "OK"; }
  else if (cmd == "ATAT0") { elmState.adaptiveTiming = false; return "OK"; }
  else if (cmd == "ATAT1") { elmState.adaptiveTiming = true; return "OK"; }
  else if (cmd == "ATAT2") { elmState.adaptiveTiming = true; return "OK"; }
  else if (cmd.startsWith("AT")) { return "OK"; } // Generic AT command
  
  return "?";
}

String OBDSimulator::processOBDPID(String mode, String pid) {
  if (mode == "01") { // Show current data
    if (pid == "00") {
      return formatResponse("41 00 BE 1F B8 11");
    }
    else if (pid == "05") {
      // Engine coolant temperature
      int temp = (int)(simData.coolantTemp + 40);
      return formatResponse("41 05 " + formatHex(temp));
    }
    else if (pid == "0C") {
      // Engine RPM
      int rpm = (int)(simData.rpm * 4);
      int a = (rpm >> 8) & 0xFF;
      int b = rpm & 0xFF;
      return formatResponse("41 0C " + formatHex(a) + " " + formatHex(b));
    }
    else if (pid == "0D") {
      // Vehicle speed
      int speed = (int)simData.speed;
      return formatResponse("41 0D " + formatHex(speed));
    }
    else if (pid == "0E") {
      // Timing advance
      int timing = 10 + 128; // 10 degrees + 128 offset
      return formatResponse("41 0E " + formatHex(timing));
    }
    else if (pid == "0F") {
      // Intake air temperature
      int temp = 25 + 40; // 25°C + 40 offset
      return formatResponse("41 0F " + formatHex(temp));
    }
    else if (pid == "10") {
      // Airflow rate
      int airflow = (int)(simData.airflowRate * 100);
      int a = (airflow >> 8) & 0xFF;
      int b = airflow & 0xFF;
      return formatResponse("41 10 " + formatHex(a) + " " + formatHex(b));
    }
    else if (pid == "11") {
      // Throttle position
      int throttle = (int)(simData.throttlePos * 2.55);
      return formatResponse("41 11 " + formatHex(throttle));
    }
    else if (pid == "0B") {
      // Intake manifold absolute pressure
      int pressure = (int)(simData.boostPressure + 101);
      pressure = constrain(pressure, 0, 255);
      return formatResponse("41 0B " + formatHex(pressure));
    }
    else if (pid == "04") {
      // Engine load
      int load = (int)(simData.engineLoad * 2.55);
      return formatResponse("41 04 " + formatHex(load));
    }
    else if (pid == "2F") {
      // Fuel level
      int fuel = (int)(simData.fuelLevel * 2.55);
      return formatResponse("41 2F " + formatHex(fuel));
    }
    else if (pid == "5C") {
      // Engine oil temperature
      int temp = (int)(simData.oilTemp + 40);
      return formatResponse("41 5C " + formatHex(temp));
    }
    else if (pid == "20") {
      return formatResponse("41 20 80 00 00 01");
    }
    else if (pid == "40") {
      return formatResponse("41 40 40 00 00 00");
    }
  }
  else if (mode == "03") { // Show stored DTCs
    return "43 00"; // No DTCs
  }
  else if (mode == "04") { // Clear DTCs
    return "44";
  }
  else if (mode == "09") { // Vehicle information
    if (pid == "00") {
      return formatResponse("49 00 54");
    }
    else if (pid == "02") {
      // VIN
      return formatResponse("49 02 01 31 44 34 47 50 30 30 42 35 36 37 35 38 39");
    }
  }
  
  return "NO DATA";
}

String OBDSimulator::formatResponse(String response) {
  if (!elmState.spacesOn && response != "NO DATA") {
    String noSpaces = response;
    noSpaces.replace(" ", "");
    return noSpaces;
  }
  return response;
}

String OBDSimulator::formatHex(int value) {
  String hex = String(value, HEX);
  hex.toUpperCase();
  if (hex.length() == 1) hex = "0" + hex;
  return hex;
}

void OBDSimulator::sendClassicResponse(String cmd, String response) {
  String fullResponse = "";
  
  if (cmd.startsWith("AT")) {
    if (elmState.echoOn) {
      fullResponse = cmd + "\r" + response + "\r\n>";
    } else {
      fullResponse = response + "\r\n>";
    }
  }
  else {
    // OBD2 data commands - no echo
    fullResponse = response + "\r\n>";
  }
  
  pSerialBT->print(fullResponse);
  delay(elmState.adaptiveTiming ? 50 : 20);
}

void OBDSimulator::sendBLEResponse(String response) {
  if (deviceConnected) {
    // Add prompt for BLE responses (except for initial prompt)
    if (response != ">") {
      response += "\r\n>";
    }
    
    pTxCharacteristic->setValue(response.c_str());
    pTxCharacteristic->notify();
    delay(20); // Small delay for BLE stability
  }
}

void OBDSimulator::setDeviceName(String classic, String ble) {
  classicBTName = classic;
  bleName = ble;
}

void OBDSimulator::resetELMState() {
  elmState.echoOn = true;
  elmState.headersOn = false;
  elmState.spacesOn = true;
  elmState.lineFeedsOn = true;
  elmState.protocol = "6";
  elmState.adaptiveTiming = true;
  elmState.timeout = 200;
}

void OBDSimulator::printSystemInfo() {
  Serial.println("🔧 System Information:");
  Serial.println("   📋 ESP32 Chip: " + String(ESP.getChipModel()));
  Serial.println("   🔢 Revision: " + String(ESP.getChipRevision()));
  Serial.println("   💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("   ⏰ CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
  Serial.println();
}

void OBDSimulator::printStatus() {
  Serial.println("📊 Current simulation values:");
  Serial.println("   🔄 RPM: " + String(simData.rpm, 0));
  Serial.println("   🏃 Speed: " + String(simData.speed, 0) + " km/h");
  Serial.println("   🌡️  Coolant: " + String(simData.coolantTemp, 1) + "°C");
  Serial.println("   🛢️  Oil: " + String(simData.oilTemp, 1) + "°C");
  Serial.println("   ⛽ Fuel: " + String(simData.fuelLevel, 1) + "%");
  Serial.println("   🔧 Load: " + String(simData.engineLoad) + "%");
  Serial.println("   💨 Throttle: " + String(simData.throttlePos, 1) + "%");
  
  String connections = "📱 Connections: ";
  if (classicConnected) connections += "Classic✅ ";
  if (bleConnected) connections += "BLE✅ ";
  if (!classicConnected && !bleConnected) connections += "None";
  Serial.println(connections);
  Serial.println();
}

// BLE Server Callbacks Implementation
void MyServerCallbacks::onConnect(BLEServer* pServer) {
  g_simulator->deviceConnected = true;
  g_simulator->bleConnected = true;
  g_simulator->connectionTime = millis();
  g_simulator->commandCount = 0;
  
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("🎉 BLE CLIENT CONNECTED!");
  Serial.println("⏰ Connection Time: " + String(millis()) + " ms");
  Serial.println("🔧 ELM327 State Reset to defaults");
  
  // Reset ELM state
  g_simulator->resetELMState();
  
  // Send initial prompt after small delay
  delay(100);
  g_simulator->sendBLEResponse(">");
  Serial.println("📤 BLE: Initial prompt '>' sent");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
  g_simulator->deviceConnected = false;
  g_simulator->bleConnected = false;
  
  unsigned long duration = millis() - g_simulator->connectionTime;
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("👋 BLE CLIENT DISCONNECTED!");
  Serial.println("⏰ Duration: " + String(duration) + " ms");
  Serial.println("📊 Commands: " + String(g_simulator->commandCount));
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  
  // Restart advertising
  BLEDevice::startAdvertising();
  Serial.println("🔍 BLE advertising restarted");
}

// BLE Characteristic Callbacks Implementation
void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
  String rxValue = pCharacteristic->getValue();
  
  if (rxValue.length() > 0) {
    // Remove any trailing newlines/carriage returns
    rxValue.trim();
    
    if (rxValue.length() > 0) {
      g_simulator->commandCount++;
      unsigned long timeSinceConnection = millis() - g_simulator->connectionTime;
      
      if (g_simulator->debugMode) {
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("📨 BLE COMMAND #" + String(g_simulator->commandCount));
        Serial.println("⏰ Time: +" + String(timeSinceConnection) + " ms");
        Serial.println("📝 Raw: '" + rxValue + "'");
      }
      
      String response = g_simulator->processOBDCommand(rxValue, "BLE");
      g_simulator->sendBLEResponse(response);
      
      if (g_simulator->debugMode) {
        Serial.println("🔄 BLE Response: '" + response + "'");
        Serial.println("✅ BLE Response sent!");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      }
    }
  }
}

// Classic Bluetooth Callback Implementation
void btClassicCallbackFunction(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  switch (event) {
    case ESP_SPP_SRV_OPEN_EVT:
      g_simulator->classicConnected = true;
      g_simulator->connectionTime = millis();
      g_simulator->commandCount = 0;
      
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      Serial.println("🎉 CLASSIC BT CLIENT CONNECTED!");
      Serial.println("⏰ Connection Time: " + String(millis()) + " ms");
      Serial.println("🔧 ELM327 State Reset to defaults");
      
      // Reset ELM state
      g_simulator->resetELMState();
      
      // Send initial prompt
      delay(100);
      g_simulator->pSerialBT->print(">");
      Serial.println("📤 Classic BT: Initial prompt '>' sent");
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      break;
      
    case ESP_SPP_CLOSE_EVT:
      {
        unsigned long duration = millis() - g_simulator->connectionTime;
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("👋 CLASSIC BT CLIENT DISCONNECTED!");
        Serial.println("⏰ Duration: " + String(duration) + " ms");
        Serial.println("📊 Commands: " + String(g_simulator->commandCount));
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        g_simulator->classicConnected = false;
      }
      break;
      
    default:
      break;
  }
}
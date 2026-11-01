#include "DeviceFunction.h"

DeviceFunction::DeviceFunction(String name, int motor,int stateDoneReq) {
    deviceName = name;
    motorPin = motor;
    stateDone = stateDoneReq;
    isRunning = false;
    
    // Configureer pins
    if (motorPin != -1) {
        pinMode(motorPin, OUTPUT);
        digitalWrite(motorPin, LOW);
    }
}

DeviceFunction::~DeviceFunction() {
    if (motorPin != -1) digitalWrite(motorPin, LOW);
}

// Run Methode voor device
void DeviceFunction::execute() {
    Serial.println("\n=== " + deviceName + " Uitvoeren ===");
    if (motorPin == -1) {
        Serial.println("FOUT: Motor pin niet geconfigureerd!");
        return;
    }

    // Run DC motor
    isRunning = true;
    Serial.println("Motor AAN (pin " + String(motorPin) + ")");
    digitalWrite(motorPin, HIGH);
    
    delay(2000); // Mix voor opgegeven duur (standaard 2 seconden)
    
    // Zet uit
    digitalWrite(motorPin, LOW);
    isRunning = false;
    Serial.println(deviceName + " klaar!");
    Serial.println("===================\n");
}

// Getters
String DeviceFunction::getDeviceName() {
    return deviceName;
}
int DeviceFunction::getStateDone() {
    return stateDone;
}
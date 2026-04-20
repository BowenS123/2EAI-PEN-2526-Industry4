#ifndef DEVICE_FUNCTION_H
#define DEVICE_FUNCTION_H

#include <Arduino.h>

class DeviceFunction {
private:
    // I/O ports 
    int motorPin;
    // custom name device (defined in custructor)
    String deviceName;
    bool isRunning = false;
    int stateDone ;
public:
    DeviceFunction(String name, int motorPin,int stateDoneReq);
    ~DeviceFunction();
    void execute();// with I/O ports
    
    // getters
    String getDeviceName();
    int getStateDone();
};
#endif
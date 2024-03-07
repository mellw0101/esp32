#ifndef BLUETOOTH_SERIAL_HANDLER_H
#define BLUETOOTH_SERIAL_HANDLER_H

#include "BluetoothSerial.h"

class BluetoothSerialHandler
{
    public:
        BluetoothSerialHandler();
        void begin(const char* name);
        String receiveText();
        bool hasText();

    private:
        BluetoothSerial serialBT;
};

#endif
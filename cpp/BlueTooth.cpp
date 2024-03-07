#include "BlueTooth.h"

BluetoothSerialHandler::BluetoothSerialHandler()
{
}

void BluetoothSerialHandler::begin(const char* name)
{
    if (!serialBT.begin(name))
    {
        Serial.println("Failed to init BlueTooth.");
    }
}

String BluetoothSerialHandler::receiveText()
{
    String received = "";
    if (serialBT.available())
    {
        received = serialBT.readString();
    }
    return received;
}

bool BluetoothSerialHandler::hasText()
{
    return serialBT.available() > 0;
}

#include <Arduino.h>
#include <stdio.h>
#include "memory.h"

rtcDataType rtcData;

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xffffffff;
    while (length--)
    {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1)
        {
            bool bit = crc & 0x80000000;
            if (c & i)
            {
                bit = !bit;
            }
            crc <<= 1;
            if (bit)
            {
                crc ^= 0x04c11db7;
            }
        }
    }
    return crc;
}

void printMemory()
{
    char buf[3];
    uint8_t *ptr = (uint8_t *)&rtcData;
    for (size_t i = 0; i < sizeof(rtcData); i++)
    {
        sprintf(buf, "%02X", ptr[i]);
        Serial.print(buf);
        if ((i + 1) % 32 == 0)
        {
            Serial.println();
            delay(50);
        }
        else
        {
            Serial.print(" ");
        }
    }
    delay(50);
    Serial.println();
}

void clearMemory()
{
    for (size_t i = 0; i < sizeof(rtcData.data); i++)
    {
        rtcData.data[i] = (byte)0;
    }
    rtcData.crc32 = 0;
};

bool setdefaultMemory()
{
    // PATTERN
    rtcData.data[0] = 0x55;
    rtcData.data[1] = 0xAA;
    rtcData.data[2] = 0x55;
    rtcData.data[3] = 0xAA;
    // MODE
    rtcData.data[4] = 0x55; // 0x55 FM vagy 0xAA = WiFi radio szoljon.
    // POINTER, radio index
    rtcData.data[5] = 0x00; //
    // DEEPSLEEP
    rtcData.data[6] = 0x55; // 0x55 all, 0xAA el van indítva
    // BATTERY
    rtcData.data[7] = (byte)26; // jelentese 2.6 Volt a minimum elem feszultseg
    rtcData.crc32 = calculateCRC32((uint8_t *)&rtcData.data[0], sizeof(rtcData.data));
    return ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtcData, sizeof(rtcData));
}

bool setvaluetMemory(byte data, int index)
{
    rtcData.data[index] = data;
    rtcData.crc32 = calculateCRC32((uint8_t *)&rtcData.data[0], sizeof(rtcData.data));
    return ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtcData, sizeof(rtcData));
}

byte getvalueMemory(int index)
{
    if (getMemory())
    {
        return rtcData.data[index];
    }
    return 0xFF;
}

bool getMemory()
{
    if (ESP.rtcUserMemoryRead(0, (uint32_t *)&rtcData, sizeof(rtcData)))
    {
        uint32_t crcOfData = calculateCRC32((uint8_t *)&rtcData.data[0], sizeof(rtcData.data));
        if (crcOfData == rtcData.crc32)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}
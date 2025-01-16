#ifndef MEMORY_H
#define MEMORY_H
#pragma once

/*
RTC RAM tartalma mindig 8 byte az alábbiak szerint:
A) PATTERN négy byte helyesen legyen:
0x55 0xAA 0x55 0xAA
B) MODE egy byte: legyen = 0x55 FM rádió szóljon
(avagy 0xAA=WiFi rádió szóljon.)
Minden egyéb kód FATAL ERROR
C) POINTER egy byte: legyen = 0x01
(amely lehet 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08
aszerint hányadik rádióállomáson áll éppen a lejátszás.)
Minden egyéb kód FATAL ERROR
D) DEEPSLEEP egy byte: legyen = 0x55
Amely lehet:
0xAA=Deep Sleep el van indítva, tehát a Reset majd a GPIO16 felôl is lesz várható, és a gomb felôl is.
0x55=Deep Sleep még nem lett elindítva, mert éppen müveletek folynak.
Minden egyéb kód FATAL ERROR
E) BATTERY egy byte: legyen 26 dec
amelynek jelentése 2.6 Volt mint minimum elem feszültség majd telefonnal állítható lesz pl,
ha hálózatról vagy ha tölthetô akkuról használják a rádiót.
*/
#define MODE 4
#define POINTER 5
#define DEEPSLEEP 6
#define BATTERY 7

typedef struct
{
    byte data[8]; // 8 byte
    uint32_t crc32;
} rtcDataType;

extern uint32_t calculateCRC32(const uint8_t *data, size_t length);
extern void printMemory();
extern void clearMemory();
extern bool setdefaultMemory();
extern bool getMemory();
extern bool setvaluetMemory(byte data, int index);
extern byte getvalueMemory(int index);

extern rtcDataType rtcData;
#endif // MEMORY_H
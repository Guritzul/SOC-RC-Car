#pragma once

#include <stdint.h>

// Structura Payload trimisa de telecomanda
#pragma pack(1)
struct Payload
{
    int throttle; // 0-1023
    int steering; // 0-1023
    bool buzz;
    bool swLeft;
    bool swRight;
};
#pragma pack()

namespace RadioRx
{
    // Initializeaza modulele radio (SPI, CE, nRF24L01 pe registri)
    void init();

    // Incearca sa receptioneze un Payload. Returneaza true daca s-a primit un pachet nou.
    bool receive(Payload &outPayload);
}

#pragma once
#include <RF24.h>

#define CE_PIN 9
#define CSN_PIN 10

struct Payload
{
    int throttle; // 0-1023
    int steering; // 0-1023
    bool buzz;
    bool swLeft;
    bool swRight;
};

void antennaInit();
bool antennaSend(const Payload &data);

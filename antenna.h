#pragma once
#include <RF24.h>

#define CE_PIN 9
#define CSN_PIN 10

struct Payload
{
    int throttle;
    int steering;
    bool buzz;
    bool swLeft;
    bool swRight;
};

void antennaInit();
bool antennaSend(const Payload &data);

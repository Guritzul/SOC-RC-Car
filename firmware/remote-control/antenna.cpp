#include "antenna.h"
#include <SPI.h>

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

void antennaInit()
{
    radio.begin();
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_LOW);
    radio.stopListening();
}

bool antennaSend(const Payload &data)
{
    return radio.write(&data, sizeof(data));
}
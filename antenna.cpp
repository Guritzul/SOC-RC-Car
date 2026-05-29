#include "antenna.h"
#include <SPI.h>

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

void antennaInit()
{
    radio.begin();
    
    // Configurații pentru alinierea cu receptorul pe registri
    radio.setAddressWidth(5);
    radio.setChannel(76);
    radio.setDataRate(RF24_1MBPS);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPALevel(RF24_PA_LOW);
    radio.setRetries(3, 15);
    
    radio.openWritingPipe(address);
    radio.stopListening();
}

bool antennaSend(const Payload &data)
{
    return radio.write(&data, sizeof(data));
}

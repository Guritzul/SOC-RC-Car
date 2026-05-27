#include "antenna.h"
#include <SPI.h>

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

void antennaInit()
{
    radio.begin();
    
    // Configurații explicite pentru alinierea perfectă cu receptorul pe registri
    radio.setAddressWidth(5);           // Lățime adresă: 5 bytes
    radio.setChannel(76);               // Canal: 76 (2.476 GHz)
    radio.setDataRate(RF24_1MBPS);      // Rata de date: 1 Mbps
    radio.setCRCLength(RF24_CRC_16);    // Control erori: CRC 16-bit (2 bytes)
    radio.setPALevel(RF24_PA_LOW);      // Putere joasă pentru teste la distanță mică
    radio.setRetries(3, 15);            // Retrimiteri automate: delay 1000us, 15 încercări
    
    radio.openWritingPipe(address);
    radio.stopListening();
}

bool antennaSend(const Payload &data)
{
    return radio.write(&data, sizeof(data));
}

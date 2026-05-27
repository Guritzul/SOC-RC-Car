#pragma once

#include <stdint.h>

// Shared payload structure representing the control packet sent to the car
struct Payload
{
    int throttle; // 0-1023 (0 = back, 512 = center, 1023 = forward)
    int steering; // 0-1023 (0 = left, 512 = center, 1023 = right)
    bool buzz;    // Buzzer state
    bool swLeft;  // Left joystick button state
    bool swRight; // Right joystick button state
};

namespace RadioTx
{
    // Initialize the SPI peripheral, CE/CSN pins, and nRF24L01 registers
    void init();

    // Sends the control payload over the air. Returns true on success (ACK received).
    bool send(const Payload &data);
}

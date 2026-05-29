#pragma once

#include <stdint.h>

#pragma pack(1)
struct Payload
{
    int throttle;
    int steering;
    bool buzz;
    bool swLeft;
    bool swRight;
};
#pragma pack()

namespace RadioTx
{
    void init();
    bool send(const Payload &data);
}

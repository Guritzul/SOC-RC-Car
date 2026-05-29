#include "joystick.h"

Joystick::Joystick(IAdc &adc, IGpio &switchGpio, uint8_t adcChannel, bool invertAxis)
    : _adc(adc), _switchGpio(switchGpio), _adcChannel(adcChannel), _invertAxis(invertAxis)
{
}

void Joystick::init()
{
    _switchGpio.initInputPullup();
}

uint16_t Joystick::readAxis()
{
    uint16_t rawValue = _adc.readChannel(_adcChannel);
    return _invertAxis ? (1023 - rawValue) : rawValue;
}

bool Joystick::isPressed()
{
    return !_switchGpio.read();
}

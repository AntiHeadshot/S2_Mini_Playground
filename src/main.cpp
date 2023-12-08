#include <Arduino.h>
#include <WiFi.h>
#include "HomeSpan.h"
#include "extras/PwmPin.h"

#define LED_PIN 15
#define CONTROLL_PIN 0

#define PWM_PIN 40
#define ON_OFF_PIN 1

struct Fan : Service::Fan
{
    SpanCharacteristic *power;
    SpanCharacteristic *speed;
    LedPin *fan;
    int _target = -1;

    Fan() : Service::Fan()
    {
        power = new Characteristic::Active();
        speed = new Characteristic::RotationSpeed(50);
        speed->setRange(0, 100, 5);

        pinMode(ON_OFF_PIN, OUTPUT);
        digitalWrite(ON_OFF_PIN, LOW);

        fan = new LedPin(40, 0, 25000U);
        fan->set(0);
    }

    boolean update()
    {
        int fanSpeed = speed->getNewVal();
        if (!power->getNewVal())
            fanSpeed = 0;

        digitalWrite(ON_OFF_PIN, fanSpeed > 0 ? HIGH : LOW);

        if (fan->fadeStatus() != LedPin::FADING)
            fan->fade(fanSpeed, 1000, LedPin::PROPORTIONAL);
        else
            _target = fanSpeed;

        return true;
    }

    void loop() override
    {
        if (_target >= 0 && fan->fadeStatus() != LedPin::FADING)
        {
            int target = _target;
            _target = -1;
            fan->fade(target, 1000, LedPin::PROPORTIONAL);
        }
    }
};

uint32_t getChipId()
{
    uint64_t chipId64 = 0;

    for (int i = 0; i < 6; i++)
    {
        chipId64 |= (((uint64_t)ESP.getEfuseMac() >> (40 - (i * 8))) & 0xff) << (i * 8);
    }

    return (uint32_t)(chipId64 & 0xFFFFFF);
}

void setup()
{
    homeSpan.setStatusPin(LED_PIN);

    homeSpan.setApSSID(("Fan " + String(getChipId(), HEX)).c_str());

    homeSpan.setControlPin(CONTROLL_PIN);

    homeSpan.setPairingCode("66601337");

    homeSpan.begin(Category::Fans, "Esp32 S2 Fan");

    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Fan();
}

void loop()
{
    homeSpan.poll();
    delay(200);
}

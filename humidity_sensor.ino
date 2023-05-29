#include <SimpleDHT.h>

SimpleDHT11 dht11;

void humidity_setup()
{
  DDRD &= ~(1 << PD4);
}

byte get_humidity(int pin)
{
    byte t = 0;
    byte humidity = 0;
    byte data[40] = {0};
    if (dht11.read(pin, &t, &humidity, data)) {
        Serial.print("Read DHT11 failed");
        return -1;
    }

    return humidity;
}

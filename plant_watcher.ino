#include <SoftwareSerial.h>
#include "plant_watcher.h"

SoftwareSerial GSMSerial(11, 10);


const int humidityPin = 7;

volatile unsigned long startMillisButton;
volatile unsigned long currentMillisButton;
volatile bool sendTrigger = false;
volatile unsigned long lastInfoSend = 0;

const long period = 30000;
const unsigned long infoPeriod = 6 * 3600; // Every 6 hours

unsigned long lastWarningSend = 0;

byte warningCounter = 0;
const byte warningTrigger = 20; 
const unsigned long warningPeriod = 4 * 3600; // Every 4 hours

float temp;
float pressure;
float altitude;
uint16_t soil_moisture;
float moisture_percentage;

byte warning_counter = 0;

byte humidity;
char buffer[192];


ISR(INT0_vect)
{
  currentMillisButton = millis();
  if ((long) (currentMillisButton - startMillisButton) >= period) {
    startMillisButton += currentMillisButton;
    sendTrigger = true;
  }
}

ISR(TIMER1_COMPA_vect) {
  if (lastInfoSend == infoPeriod) {
    sendTrigger = true;
    lastInfoSend = 0;
  }
  lastInfoSend++;
}

void configure_timer1() {
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 15624;
  TCCR1B |= (1 << CS10) | (1 << CS12) | (1 << WGM12); 
}


void init_timer1() {
  TIMSK1 |= (1 << OCIE1A);
}

void setup_interrupts() {
  cli();

  configure_timer1();
  init_timer1();
  
  DDRD &= ~(1 << PD2);
  PORTD |= (1 << PD2);

  EICRA |= (1 << ISC00);
  EIMSK |= (1 << INT0);

  sei();
}

void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    GSMSerial.write(Serial.read());
  }
  while(GSMSerial.available()) 
  {
    Serial.write(GSMSerial.read());
  }
}

void setup_gsm() {
  GSMSerial.begin(9600);
  GSMSerial.println("AT");
  updateSerial();
  GSMSerial.println("AT+CSQ");
  updateSerial();
  GSMSerial.println("AT+CMGF=1");
  updateSerial();
}

void setup_sensors() {
  soil_setup();
  bmp_setup();
  humidity_setup();
}

void setup() {
  Serial.begin(9600);
  setup_sensors();
  setup_gsm();
  setup_interrupts();
  
  startMillisButton = millis();
}

void add_info(char *buffer, const char *name, const char *unit, float value) {
  const char *sign = (value < 0) ? "-"  : "";
  float aux = value < 0 ? -value : value;

  int integer_part = aux;
  float frac_part = value - integer_part;
  int frac_int_part = trunc(frac_part * 100);

  sprintf(buffer, "%s%s = %s%d.%02d %s\n", buffer, name, sign, integer_part, frac_int_part, unit);
}

void loop() {
  strcpy(buffer, "");
  
  BMP280_output bmp_output;
  bmp_gather_data(&bmp_output);

  temp = bmp_output.temp;
  pressure = bmp_output.pressure;
  altitude = bmp_output.altitude;

  soil_moisture = get_soil_moisture();
  humidity = get_humidity(humidityPin);

  add_info(buffer, "Temperature", "Celsius degrees", temp);  
  add_info(buffer, "Air pressure", "HPa", pressure / 100);
  add_info(buffer, "Altitude", "meters", altitude);
  if (soil_moisture > 1000) {
    sprintf(buffer, "%sSoil sensor probably not in soil!\n", buffer);
  } else {
    moisture_percentage = 100 - ((soil_moisture / 1023.00) * 100);
    add_info(buffer, "Soil moisture", "%", moisture_percentage);
  }

  add_info(buffer, "Humidity", "%", humidity);
  
  if (!sendTrigger && (temp < TEMP_LOW_THRESHOLD || temp > TEMP_HI_THRESHOLD)) {
    warningCounter++;
    if (warningCounter == warningTrigger) {
      if (lastWarningSend == 0 || (long) (millis() - lastWarningSend) >= warningPeriod) {
        sprintf(buffer, "%sWARNING: TEMPERATURE!\n", buffer);
        sendTrigger = true;
        lastWarningSend += millis();
      }
      warningCounter = 0;
    }
  }

  if (!sendTrigger && (moisture_percentage < SOIL_LOW_THRESHOLD || moisture_percentage > SOIL_HI_THRESHOLD)) {
    warningCounter++;
    if (warningCounter == warningTrigger) {
      if (lastWarningSend == 0 || (long) (millis() - lastWarningSend) >= warningPeriod) {
        sprintf(buffer, "%sWARNING: SOIL MOISTURE!\n", buffer);
        sendTrigger = true;
        lastWarningSend += millis();
      }
      warningCounter = 0;
    }
  }
  
  if (sendTrigger) {
    GSMSerial.println("AT+CMGS=\"+407xxxxxxxx\"");
    updateSerial();
    GSMSerial.println(buffer);
    updateSerial();
    GSMSerial.write(26);
    updateSerial();
    sendTrigger = false;
  }
  
 
  delay(2000);
}

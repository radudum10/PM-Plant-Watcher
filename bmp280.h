#ifndef bmp280_h
#define bmp280_h


// HPA is prefered for avoiding precision problems / overflows
#define BLACK_SEA_LEVEL_HPA 1013

struct BMP280_compensation_t {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
};

struct BMP280_output {
    float temp;
    float pressure;
    float altitude;
};

void bmp_setup();
void bmp_gather_data(BMP280_output *out);

#endif

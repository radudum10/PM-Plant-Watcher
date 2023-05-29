#include "bmp280.h"
#include "i2c_utils.h"


int32_t bmp280_t_fine;
BMP280_compensation_t compensation;
int address;

void bmp_setup() {
    I2C_setup();

    address = I2C_Scan();
}

/* 3.11.3 Compensation formula from the datasheet */
int32_t BMP280_CompensateTemp_32(int32_t raw_value, struct BMP280_compensation_t *compensation)
{
  int32_t var1, var2, T;

  var1 = ((((raw_value>>3) - ((int32_t)compensation->dig_T1 << 1))) * ((int32_t)compensation->dig_T2)) >> 11;
  var2 = (((((raw_value >> 4) - ((int32_t)compensation->dig_T1)) * ((raw_value >> 4) - ((int32_t)compensation->dig_T1))) >> 12) *
    ((int32_t)compensation->dig_T3)) >> 14;
  bmp280_t_fine = var1 + var2;
  T = (bmp280_t_fine * 5 + 128) >> 8;
  return T; 
}

uint32_t BMP280_CompensatePressure_64(int32_t raw_value, struct BMP280_compensation_t *compensation)
{
  int64_t var1, var2, p;
  
  var1 = ((int64_t)bmp280_t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)compensation->dig_P6;
  var2 = var2 + ((var1 * (int64_t)compensation->dig_P5) << 17);
  var2 = var2 + (((int64_t)compensation->dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)compensation->dig_P3) >> 8) + ((var1 * (int64_t)compensation->dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)compensation->dig_P1)>>33;
  if (var1 == 0) {
    return 0; // avoid division by zero
  }
  p = 1048576 - raw_value;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)compensation->dig_P9) * (p>>13) * (p >> 13)) >> 25;
  var2 = (((int64_t)compensation->dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)compensation->dig_P7) << 4);
  return (uint32_t)p;
}

 void bmp_gather_data(struct BMP280_output *out) {
  int res;
  
    uint8_t raw_compensation[24];
    uint8_t id_value;
    res = I2C_RegisterRead(address, 0xD0, &id_value, 1);
    _I2C_TRANSACTION_CHECK(res, "reading ID register");
    res = I2C_RegisterRead(address, 0x88, raw_compensation, 24);
    _I2C_TRANSACTION_CHECK(res, "reading compensation registers");

    if (!compensation.dig_T1 || !compensation.dig_P1) {
      compensation.dig_T1 = ((uint16_t)raw_compensation[0] | ((uint16_t)raw_compensation[1] << 8));
      compensation.dig_T2 = (int16_t)((uint16_t)raw_compensation[2] | ((uint16_t)raw_compensation[3] << 8));
      compensation.dig_T3 = (int16_t)((uint16_t)raw_compensation[4] | ((uint16_t)raw_compensation[5] << 8));
      
      compensation.dig_P1 = ((uint16_t)raw_compensation[6] | ((uint16_t)raw_compensation[7] << 8));
      compensation.dig_P2 = (int16_t)((uint16_t)raw_compensation[8] | ((uint16_t)raw_compensation[9] << 8));
      compensation.dig_P3 = (int16_t)((uint16_t)raw_compensation[10] | ((uint16_t)raw_compensation[11] << 8));
      compensation.dig_P4 = (int16_t)((uint16_t)raw_compensation[12] | ((uint16_t)raw_compensation[13] << 8));
      compensation.dig_P5 = (int16_t)((uint16_t)raw_compensation[14] | ((uint16_t)raw_compensation[15] << 8));
      compensation.dig_P6 = (int16_t)((uint16_t)raw_compensation[16] | ((uint16_t)raw_compensation[17] << 8));
      compensation.dig_P7 = (int16_t)((uint16_t)raw_compensation[18] | ((uint16_t)raw_compensation[19] << 8));
      compensation.dig_P8 = (int16_t)((uint16_t)raw_compensation[20] | ((uint16_t)raw_compensation[21] << 8));
      compensation.dig_P9 = (int16_t)((uint16_t)raw_compensation[22] | ((uint16_t)raw_compensation[23] << 8));
    }
      
    uint8_t ctrl_meas_data = (0x01 << 5) | (0x04 << 2) | 0x03; // ctrl_meas: osrs_t[2:0] | osrs_p[2:0] | mode[1:0]
    res = I2C_RegisterWrite(address, 0xF4, &ctrl_meas_data, 1);
    _I2C_TRANSACTION_CHECK(res, "write ctrl_meas reg");
    
    uint8_t config_data = (0x01 << 5) | (0x04 << 2) | 0x01; // standby[2:0] | filter[2:0] | - | spi3w_en (3/4 wire I2C) 
    res = I2C_RegisterWrite(address, 0xF5, &config_data, 1);
    _I2C_TRANSACTION_CHECK(res, "write config reg");
  
  uint8_t raw_temp[3];
  res = I2C_RegisterRead(address, 0xFA, raw_temp, 3);
  _I2C_TRANSACTION_CHECK(res, "reading temp registers");
  int32_t temp32 = (int32_t)(((uint32_t)raw_temp[0] << 12) | ((uint32_t)raw_temp[1] << 4)
    | (uint32_t)(raw_temp[2] & 0x0F));
  int32_t temperature = BMP280_CompensateTemp_32(temp32, &compensation);
  out->temp = (float) temperature / 100;

  uint8_t raw_pressure[3];
  res = I2C_RegisterRead(address, 0xF7, raw_pressure, 3);
  _I2C_TRANSACTION_CHECK(res, "reading pressure registers");
  int32_t pressure32 = (int32_t) (((uint32_t)raw_pressure[0] << 12) | ((uint32_t) raw_pressure[1] << 4) | (uint32_t)(raw_pressure[2] & 0x0F));
  uint32_t pressure = BMP280_CompensatePressure_64(pressure32, &compensation);
  out->pressure = (float) pressure / 256;

  out->altitude = 44330 * (1.0 - pow(out->pressure / 100 / BLACK_SEA_LEVEL_HPA, 0.1903));
}

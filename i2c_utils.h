#ifndef I2C_utils_h
#define I2C_utils_h

#define I2C_TIMEOUT 20
#define I2C_READ 1
#define I2C_WRITE 0 
#define I2C_ERR_TIMEOUT -1
#define I2C_ERR_START -2
#define I2C_ERR_ACK -3

#define _I2C_TRANSACTION_CHECK(res, _text) { \
  if ((res) < 0) { \
    Serial.print("I2C error: " _text); Serial.println(res); \
    delay(5000); return; } }


void I2C_setup();
uint8_t I2C_waitForInterrupt();
int8_t I2C_start(uint8_t address, uint8_t mode);
void I2C_stop();
int8_t I2C_write(uint8_t data);
int16_t I2C_read(uint8_t ack_more);
int I2C_Scan();
int I2C_RegisterWrite(uint8_t addr, uint8_t reg_id, uint8_t *data, uint8_t data_len);
int I2C_RegisterRead(uint8_t addr, uint8_t reg_id, uint8_t *data_out, uint8_t data_len);


#endif
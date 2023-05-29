#include <util/twi.h>


void I2C_setup() {
  TWSR = (0b00 << TWPS0);
  TWBR = 72;
}

uint8_t I2C_waitForInterrupt() {
  uint8_t i = 0;
  while (!(TWCR & (1<<TWINT))) {
    if (i >= I2C_TIMEOUT) return 0;
    _delay_us(50); i++;
  }
  return 1;
}

int8_t I2C_start(uint8_t address, uint8_t mode) {
  TWCR = 0;
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
  
  if (!I2C_waitForInterrupt())
    return I2C_ERR_TIMEOUT;

  uint8_t twst = (TWSR & TW_STATUS_MASK);
  if (twst != TW_START) {
    return I2C_ERR_START;
  }

  TWDR = (((address) << 1) | (mode));

  TWCR = (1<<TWINT) | (1<<TWEN);
  if (!I2C_waitForInterrupt())
    return I2C_ERR_TIMEOUT;

  twst = (TWSR & TW_STATUS_MASK);
  if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK))
    return I2C_ERR_ACK;
  return 0;
}

void I2C_stop() {
  TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN) ;
}

int8_t I2C_write(uint8_t data) {
  TWDR = data;
  TWCR = (1<<TWINT) | (1<<TWEN);
  if (!I2C_waitForInterrupt())
     return I2C_ERR_TIMEOUT;
  if ((TWSR & TW_STATUS_MASK) != TW_MT_DATA_ACK)
     return I2C_ERR_ACK;
  return 0;
}

int16_t I2C_read(uint8_t ack_more) {
  TWCR = (1<<TWINT) | (1<<TWEN) | (ack_more ? (1 << TWEA) : 0);
  if (!I2C_waitForInterrupt())
    return I2C_ERR_TIMEOUT;
  return TWDR;
}

int I2C_Scan()
{
  uint8_t foundAddress = 0;
  int res;
  for (uint8_t addr=1; addr <= 0x7F; addr++) {
    res = I2C_start(addr, I2C_WRITE);
    I2C_stop();
    if (res == 0) {
      foundAddress = addr;
    }
  }
  return foundAddress;
}

int I2C_RegisterWrite(uint8_t addr, uint8_t reg_id, uint8_t *data, uint8_t data_len)
{
  int res;
  if (I2C_start(addr, I2C_WRITE) < 0) return -1;
  I2C_write(reg_id);
  for (int i=0; i<data_len; i++) {
    res = I2C_write(data[i]);
    if (res < 0) { I2C_stop(); return res; }
  }

  I2C_stop();
  return 0;
}

int I2C_RegisterRead(uint8_t addr, uint8_t reg_id, uint8_t *data_out, uint8_t data_len)
{
  int res;
  if (I2C_start(addr, I2C_WRITE) < 0) return -1;
  I2C_write(reg_id);
  if (I2C_start(addr, I2C_READ) < 0) return -1;
  for (int i=0; i<data_len; i++) {
    res = I2C_read((i < (data_len - 1)));
    if (res >= 0) data_out[i] = res;
  }
  I2C_stop();
  return 0;
}

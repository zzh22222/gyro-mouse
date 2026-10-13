#include <Wire.h>

#define MPU 0x68

int16_t s16(byte h, byte l) { return (int16_t)((h << 8) | l); }

void wr(byte r, byte v) {
  Wire.beginTransmission(MPU); Wire.write(r); Wire.write(v); Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  delay(200);
  wr(0x6B, 0x00);
  wr(0x1B, 0x00);
  wr(0x1A, 0x03);
  delay(100);
  Serial.println("e1 = I2C 写事务返回码(0=成功)  n = 读回字节数(必须=14)");
  Serial.println("把模块放平、什么都别碰，看下面这些行");
}

void loop() {
  byte d[14];
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  byte e1 = Wire.endTransmission(false);
  byte n = Wire.requestFrom((int)MPU, (int)14);
  for (int i = 0; i < 14; i++) d[i] = Wire.available() ? Wire.read() : 0;

  Serial.printf("e1=%d n=%2d | gx %8.2f  gy %8.2f  gz %8.2f\n",
                e1, n,
                s16(d[8],  d[9])  / 131.0,
                s16(d[10], d[11]) / 131.0,
                s16(d[12], d[13]) / 131.0);
  delay(500);
}

#include <Wire.h>

#define MPU 0x68

void wr(byte r, byte v) {
  Wire.beginTransmission(MPU); Wire.write(r); Wire.write(v); Wire.endTransmission();
}

void readBlock(byte r, byte *b, byte n) {
  Wire.beginTransmission(MPU); Wire.write(r);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU, (int)n);
  for (byte i = 0; i < n; i++) b[i] = Wire.available() ? Wire.read() : 0;
}

int16_t s16(byte h, byte l) { return (int16_t)((h << 8) | l); }

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  delay(200);
  wr(0x6B, 0x00);   // 退出 sleep
  wr(0x1C, 0x00);   // 加速 ±2g -> 16384 LSB/g
  wr(0x1A, 0x03);
  delay(100);
  Serial.println("放稳后读三个数，哪个接近 +/-1.0 就是重力所在的那根轴");
}

void loop() {
  byte d[6];
  readBlock(0x3B, d, 6);
  Serial.printf("ax %6.2f  ay %6.2f  az %6.2f\n",
                s16(d[0], d[1]) / 16384.0,
                s16(d[2], d[3]) / 16384.0,
                s16(d[4], d[5]) / 16384.0);
  delay(500);
}

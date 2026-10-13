#include <Wire.h>

#define MPU 0x68
#define N 500

void wr(byte r, byte v) {
  Wire.beginTransmission(MPU); Wire.write(r); Wire.write(v); Wire.endTransmission();
}

void readBlock(byte r, byte* b, byte n) {
  Wire.beginTransmission(MPU); Wire.write(r);
  Wire.endTransmission(false);
  Wire.requestFrom((int)MPU, (int)n);
  for (byte i = 0; i < n; i++) b[i] = Wire.available() ? Wire.read() : 0;
}

int16_t s16(byte h, byte l) { return (int16_t)((h << 8) | l); }

float bg[3];

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  delay(200);
  wr(0x6B, 0x00);   // 退出 sleep
  wr(0x1B, 0x00);   // 陀螺 ±250 dps
  wr(0x1A, 0x03);   // DLPF 44Hz
  delay(300);

  Serial.println("别碰模块，采零偏中...");
  double sum[3] = {0, 0, 0};
  byte d[14];
  for (int i = 0; i < N; i++) {
    readBlock(0x3B, d, 14);
    sum[0] += s16(d[8],  d[9]);
    sum[1] += s16(d[10], d[11]);
    sum[2] += s16(d[12], d[13]);
    delay(4);
  }
  for (byte a = 0; a < 3; a++) bg[a] = sum[a] / N;
  Serial.println("校准完成，开始打印");
}

void loop() {
  static byte n = 0;
  if (++n >= 20) {
    n = 0;
    byte d[14];
    readBlock(0x3B, d, 14);
    float gx = (s16(d[8],  d[9])  - bg[0]) / 131.0;
    float gy = (s16(d[10], d[11]) - bg[1]) / 131.0;
    float gz = (s16(d[12], d[13]) - bg[2]) / 131.0;
    Serial.printf("gx %7.2f  gy %7.2f  gz %7.2f\n", gx, gy, gz);
  }
  delay(2);
}

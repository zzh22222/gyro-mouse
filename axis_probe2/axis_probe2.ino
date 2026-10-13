#include <Wire.h>

#define MPU 0x68

const char *NAME[3] = {"gx", "gy", "gz"};
float bg[3], cur[3];

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

void readGyro() {
  byte d[14];
  readBlock(0x3B, d, 14);
  cur[0] = (s16(d[8],  d[9])  - bg[0]) / 131.0;
  cur[1] = (s16(d[10], d[11]) - bg[1]) / 131.0;
  cur[2] = (s16(d[12], d[13]) - bg[2]) / 131.0;
}

void window(const char *title, const char *howto, int secs) {
  Serial.print("\n>>> ");
  Serial.println(title);
  Serial.print("    怎么做: ");
  Serial.println(howto);
  Serial.println("    3 秒后开始记录...");
  delay(3000);

  float mx[3] = {-9999, -9999, -9999};
  float mn[3] = { 9999,  9999,  9999};
  unsigned long t0 = millis();
  while (millis() - t0 < (unsigned long)secs * 1000UL) {
    readGyro();
    for (byte a = 0; a < 3; a++) {
      if (cur[a] > mx[a]) mx[a] = cur[a];
      if (cur[a] < mn[a]) mn[a] = cur[a];
    }
    delay(2);
  }

  byte win = 0;
  for (byte a = 1; a < 3; a++)
    if (mx[a] - mn[a] > mx[win] - mn[win]) win = a;

  for (byte a = 0; a < 3; a++) {
    Serial.printf("    %s  %8.2f .. %8.2f", NAME[a], mn[a], mx[a]);
    if (a == win) Serial.print("   <== 主响应轴");
    Serial.println();
  }
  delay(1500);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  delay(200);
  wr(0x6B, 0x00);
  wr(0x1B, 0x00);
  wr(0x1A, 0x03);
  delay(300);

  Serial.println("采零偏，别碰模块...");
  double sum[3] = {0, 0, 0};
  byte d[14];
  for (int i = 0; i < 500; i++) {
    readBlock(0x3B, d, 14);
    sum[0] += s16(d[8],  d[9]);
    sum[1] += s16(d[10], d[11]);
    sum[2] += s16(d[12], d[13]);
    delay(4);
  }
  for (byte a = 0; a < 3; a++) bg[a] = sum[a] / 500.0;
  Serial.printf("bias %.2f %.2f %.2f\n", bg[0] / 131.0, bg[1] / 131.0, bg[2] / 131.0);

  Serial.println("\n把模块【平放桌面、印字朝上、长边对着你自己】");
  Serial.println("接下来 30 秒里只做屏幕让你做的事，一次一个动作");
  delay(4000);

  window("动作A：顺时针水平转（像转盘子，不要倾斜）", "顺时针转一下，再转回原位", 5);
  window("动作B：把左边缘抬起来", "抬起约 30 度，停一下，放回桌面", 5);
  window("动作C：把靠近你的这一端抬起来", "抬起约 30 度，停一下，放回桌面", 5);

  Serial.println("\n=== 测完了，把上面三段原样发给我 ===");
  while (true) delay(1000);
}

void loop() {}

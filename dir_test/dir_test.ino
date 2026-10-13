#include <Wire.h>

#define MPU 0x68
#define N 500

#define X_AXIS 2      // gz -> 光标左右
#define Y_AXIS 1      // gy -> 光标上下
#define PXPS 15.0f    // 虚拟灵敏度: 1 dps 持续 1 秒 = 15 "像素"

#define STILL_DPS 0.8f
#define STILL_MS  1200
#define PRINT_MS  1000
#define TRK       0.005f   // 静止时吸收零偏的速度(时间常数约 1 秒)

float bg[3], raw[3];

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
  raw[0] = (s16(d[8],  d[9])  - bg[0]) / 131.0;
  raw[1] = (s16(d[10], d[11]) - bg[1]) / 131.0;
  raw[2] = (s16(d[12], d[13]) - bg[2]) / 131.0;
}

// 累加器 = 光标会走的位置。X 正=往右, Y 正=往下
float accX = 0, accY = 0;
int maxAX = 0, minAX = 0, maxAY = 0, minAY = 0;
unsigned long tStill = 0, tPrint = 0;
bool moving = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  delay(200);
  wr(0x6B, 0x00);
  wr(0x1B, 0x00);
  wr(0x1A, 0x03);
  delay(300);

  Serial.println("摆放: 模块长边指向屏幕, 印字朝上, 贴在厚书上, 书平放桌面");
  Serial.println("别碰桌子/别碰书, 采零偏中...");
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
  Serial.printf("bias %.2f %.2f %.2f\n", bg[0] / 131.0, bg[1] / 131.0, bg[2] / 131.0);

  Serial.println("约定: X 正=往右, Y 正=往下");
  Serial.println("静止时会自动吸收残差(不漂了). 停住1.2秒=一个动作结束, 打结论并归零.");
  Serial.println("判据: 纯水平转时 gz 必须跳到 5 以上; 若 gz 一直贴在+-2 而动的是 gy, 那一下其实是倾斜不是水平转.");
}

void printGesture() {
  Serial.println("---- 动作结束 ----");
  Serial.printf("  X(左右) 最正 %d  最负 %d\n", maxAX, minAX);
  Serial.printf("  Y(上下) 最正 %d  最负 %d\n", maxAY, minAY);
}

void loop() {
  const float dt = 0.005;

  readGyro();

  bool active = fabsf(raw[0]) > STILL_DPS || fabsf(raw[1]) > STILL_DPS || fabsf(raw[2]) > STILL_DPS;
  if (active) {
    moving = true;
    tStill = millis();
  } else {
    // 静止: 把残差慢慢并进零偏里, 这是消漂移的关键
    for (byte a = 0; a < 3; a++) bg[a] += raw[a] * 131.0f * TRK;
    if (moving && millis() - tStill >= STILL_MS) {
      printGesture();
      accX = 0; accY = 0;
      maxAX = minAX = maxAY = minAY = 0;
      moving = false;
    }
    if (!moving) { accX = 0; accY = 0; }
  }

  if (moving) {
    accX += raw[X_AXIS] * dt * PXPS;
    accY += raw[Y_AXIS] * dt * PXPS;
    int ax = (int)accX, ay = (int)accY;
    if (ax > maxAX) maxAX = ax;
    if (ax < minAX) minAX = ax;
    if (ay > maxAY) maxAY = ay;
    if (ay < minAY) minAY = ay;
  }

  if (millis() - tPrint >= PRINT_MS) {
    tPrint = millis();
    Serial.printf("gz %6.1f X %5d  |  gy %6.1f Y %5d  | 残差 %.2f %.2f\n",
                  raw[X_AXIS], (int)accX, raw[Y_AXIS], (int)accY, raw[0], raw[2]);
  }
  delay(5);
}

#include <Wire.h>
#include "USB.h"
#include "USBHIDMouse.h"

#define MPU 0x68
#define N 500

#define X_AXIS    2
#define X_SIGN   -1
#define Y_AXIS    1
#define Y_SIGN   -1
#define SENS      40.0f
#define DEADZONE  0.3f
#define TRK       0.005f
#define STILL_DPS 1.5f
#define STILL_MS  1200
#define PRINT_MS  1000

#define ENC_A   7
#define ENC_B   6
#define HOLD_MS 250

USBHIDMouse TheMouse;
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
  raw[0] = (s16(d[8], d[9]) - bg[0]) / 131.0;
  raw[1] = (s16(d[10], d[11]) - bg[1]) / 131.0;
  raw[2] = (s16(d[12], d[13]) - bg[2]) / 131.0;
}

float carryX = 0, carryY = 0, accX = 0, accY = 0;
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

  Serial.println("摆放: 模块长边指向屏幕, 印字朝上, 贴在厚书上");
  Serial.println("别碰桌子/别碰书, 采零偏中...");
  double sum[3] = {0, 0, 0};
  byte d[14];
  for (int i = 0; i < N; i++) {
    readBlock(0x3B, d, 14);
    sum[0] += s16(d[8], d[9]);
    sum[1] += s16(d[10], d[11]);
    sum[2] += s16(d[12], d[13]);
    delay(4);
  }
  for (byte a = 0; a < 3; a++) bg[a] = sum[a] / N;
  Serial.printf("bias %.2f %.2f %.2f\n", bg[0] / 131.0, bg[1] / 131.0, bg[2] / 131.0);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  TheMouse.begin();
  USB.begin();
  delay(500);

  Serial.println("编码器开火版. A->GPIO7 B->GPIO6 C->GND, VCC 不接");
  Serial.println("顺时针转=左键(连转=连发), 逆时针不打火");
}

void loop() {
  const float dt = 0.005;
  readGyro();

  bool active = fabsf(raw[X_AXIS]) > STILL_DPS || fabsf(raw[Y_AXIS]) > STILL_DPS;
  if (active) {
    moving = true;
    tStill = millis();
  } else if (millis() - tStill >= STILL_MS) {
    for (byte a = 0; a < 3; a++) bg[a] += raw[a] * 131.0f * TRK;
    if (!moving) { accX = 0; accY = 0; }
    moving = false;
  }

  static bool lastA = true;
  static unsigned long tFire = 0;
  static int nFwd = 0, nBack = 0;
  bool a = (digitalRead(ENC_A) == LOW);
  if (a && !lastA) {
    if (digitalRead(ENC_B) == HIGH) { tFire = millis(); nFwd++; }
    else nBack++;
  }
  lastA = a;

  static bool fired = false;
  bool want = (tFire != 0) && (millis() - tFire < HOLD_MS);
  if (want != fired) {
    fired = want;
    if (fired) { TheMouse.press(MOUSE_LEFT); Serial.println("== FIRE on =="); }
    else { TheMouse.release(MOUSE_LEFT); Serial.println("== FIRE off =="); }
  }

  if (moving) {
    float rx = raw[X_AXIS] * X_SIGN;
    float ry = raw[Y_AXIS] * Y_SIGN;
    if (fabsf(rx) < DEADZONE) rx = 0;
    if (fabsf(ry) < DEADZONE) ry = 0;
    carryX += rx * dt * SENS;
    carryY += ry * dt * SENS;
    int dx = (int)carryX, dy = (int)carryY;
    if (dx != 0 || dy != 0) {
      carryX -= dx;
      carryY -= dy;
      TheMouse.move(constrain(dx, -127, 127), constrain(dy, -127, 127));
      accX += dx;
      accY += dy;
    }
  }

  if (millis() - tPrint >= PRINT_MS) {
    tPrint = millis();
    Serial.printf("gz %6.1f gy %6.1f | 状%d | 本段光标 X %5d Y %5d | 顺 %d 逆 %d 开火%d\n",
                  raw[X_AXIS], raw[Y_AXIS], (int)moving, (int)accX, (int)accY,
                  nFwd, nBack, (int)fired);
  }
  delay(5);
}

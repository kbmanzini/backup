#include <TFT_eSPI.h>

// -------------------- PINS --------------------
#define SOIL1_PIN A2
#define SOIL2_PIN A1
#define SOIL3_PIN A0

#define TEMP1_PIN A5
#define TEMP2_PIN A4
#define TEMP3_PIN A3

#define PUMP1_PIN 2
#define PUMP2_PIN 3
#define PUMP3_PIN 4

#define GREEN_LED 6
#define RED_LED   7

// -------------------- SETTINGS --------------------
const float ADC_MAX = 16383.0;

int SOIL_DRY = 1800;
int SOIL_WET = 3400;

const float BETA = 3950.0;
const float R0   = 10000.0;
const float T0   = 298.15;

int threshold = 35;

// -------------------- OBJECTS --------------------
TFT_eSPI tft = TFT_eSPI();

// -------------------- VARIABLES --------------------
int soil1, soil2, soil3;
float temp1, temp2, temp3;

bool pump1 = false;
bool pump2 = false;
bool pump3 = false;

unsigned long pump1Start = 0;
unsigned long pump2Start = 0;
unsigned long pump3Start = 0;

int pump1Duration = 0;
int pump2Duration = 0;
int pump3Duration = 0;

unsigned long lastSensorRead = 0;
unsigned long lastPageSwitch = 0;
const unsigned long SENSOR_INTERVAL = 5000;
const unsigned long PAGE_INTERVAL   = 4000;

int currentPage = 0;

// -------------------- PUMP DURATION --------------------
int getPumpDuration(int soilPercent) {
  if (soilPercent < 15) return 8000;
  if (soilPercent < 25) return 5000;
  return 2000;
}

// -------------------- SENSOR FUNCTIONS --------------------
int readSoil(int pin) {
  int raw = analogRead(pin);
  Serial.print("Raw soil pin "); Serial.print(pin); Serial.print(": "); Serial.println(raw);
  int percent = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
  return constrain(percent, 0, 100);
}

float readTemp(int pin) {
  int raw = analogRead(pin);
  Serial.print("Raw temp pin "); Serial.print(pin); Serial.print(": "); Serial.println(raw);
  if (raw <= 100) return 0.0;
  float resistance = R0 * (ADC_MAX - raw) / raw;
  if (resistance <= 0) return 0.0;
  float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(resistance / R0));
  return tempK - 273.15;
}

// -------------------- PUMP CONTROL --------------------
void setPump(int pin, bool &state, bool on) {
  digitalWrite(pin, on ? LOW : HIGH);
  state = on;
}

void stopAllPumps() {
  setPump(PUMP1_PIN, pump1, false);
  setPump(PUMP2_PIN, pump2, false);
  setPump(PUMP3_PIN, pump3, false);
}

// -------------------- AUTO WATER --------------------
void autoWater() {
  if (soil1 < threshold && !pump1) {
    pump1Duration = getPumpDuration(soil1);
    pump1Start = millis();
    setPump(PUMP1_PIN, pump1, true);
    Serial.print("Pump 1 ON for "); Serial.print(pump1Duration / 1000); Serial.println("s");
  }
  if (soil2 < threshold && !pump2) {
    pump2Duration = getPumpDuration(soil2);
    pump2Start = millis();
    setPump(PUMP2_PIN, pump2, true);
    Serial.print("Pump 2 ON for "); Serial.print(pump2Duration / 1000); Serial.println("s");
  }
  if (soil3 < threshold && !pump3) {
    pump3Duration = getPumpDuration(soil3);
    pump3Start = millis();
    setPump(PUMP3_PIN, pump3, true);
    Serial.print("Pump 3 ON for "); Serial.print(pump3Duration / 1000); Serial.println("s");
  }
}

// -------------------- PUMP TIMER CHECK --------------------
void checkPumpTimers() {
  unsigned long now = millis();
  if (pump1 && now - pump1Start >= (unsigned long)pump1Duration) {
    setPump(PUMP1_PIN, pump1, false);
    Serial.println("Pump 1 OFF");
  }
  if (pump2 && now - pump2Start >= (unsigned long)pump2Duration) {
    setPump(PUMP2_PIN, pump2, false);
    Serial.println("Pump 2 OFF");
  }
  if (pump3 && now - pump3Start >= (unsigned long)pump3Duration) {
    setPump(PUMP3_PIN, pump3, false);
    Serial.println("Pump 3 OFF");
  }
}

// -------------------- LED STATUS --------------------
void updateLED() {
  bool anyDry = (soil1 < threshold || soil2 < threshold || soil3 < threshold);
  digitalWrite(RED_LED,   anyDry ? HIGH : LOW);
  digitalWrite(GREEN_LED, anyDry ? LOW  : HIGH);
}

// -------------------- DRAW ARC DIAL --------------------
// cx, cy = center, r = radius, value = current, maxVal = max
// label = title, unit = unit string, color = arc color
void drawDial(int cx, int cy, int r, float value, float maxVal,
              const char* label, const char* unit, uint16_t color) {

  // Background arc (grey)
  for (int angle = 135; angle <= 405; angle += 2) {
    float rad = angle * PI / 180.0;
    int x1 = cx + (r - 4) * cos(rad);
    int y1 = cy + (r - 4) * sin(rad);
    int x2 = cx + r * cos(rad);
    int y2 = cy + r * sin(rad);
    tft.drawLine(x1, y1, x2, y2, TFT_DARKGREY);
  }

  // Value arc (colored)
  float pct = constrain(value / maxVal, 0.0, 1.0);
  int endAngle = 135 + (int)(pct * 270);
  for (int angle = 135; angle <= endAngle; angle += 2) {
    float rad = angle * PI / 180.0;
    int x1 = cx + (r - 4) * cos(rad);
    int y1 = cy + (r - 4) * sin(rad);
    int x2 = cx + r * cos(rad);
    int y2 = cy + r * sin(rad);
    tft.drawLine(x1, y1, x2, y2, color);
  }

  // Center value text
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(cx - 12, cy - 6);
  tft.print((int)value);
  tft.print(unit);

  // Label below
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(cx - (strlen(label) * 3), cy + r - 10);
  tft.print(label);
}

// -------------------- DRAW SOIL PAGE --------------------
void drawSoilPage() {
  tft.fillScreen(TFT_BLACK);

  // Title
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 4);
  tft.print("SOIL MOISTURE");

  // 3 dials across, center them
  uint16_t c1 = soil1 < threshold ? TFT_RED : TFT_GREEN;
  uint16_t c2 = soil2 < threshold ? TFT_RED : TFT_GREEN;
  uint16_t c3 = soil3 < threshold ? TFT_RED : TFT_GREEN;

  drawDial(40,  130, 35, soil1, 100, "P1", "%", c1);
  drawDial(120, 130, 35, soil2, 100, "P2", "%", c2);
  drawDial(200, 130, 35, soil3, 100, "P3", "%", c3);

  // Pump status
  tft.setTextSize(1);
  tft.setCursor(5, 195);
  tft.setTextColor(pump1 ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
  tft.print("P1:");
  tft.print(pump1 ? "ON " : "OFF");

  tft.setTextColor(pump2 ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
  tft.print(" P2:");
  tft.print(pump2 ? "ON " : "OFF");

  tft.setTextColor(pump3 ? TFT_CYAN : TFT_DARKGREY, TFT_BLACK);
  tft.print(" P3:");
  tft.print(pump3 ? "ON " : "OFF");

  // Status bar
  bool anyDry = (soil1 < threshold || soil2 < threshold || soil3 < threshold);
  tft.fillRect(0, 210, 240, 30, anyDry ? TFT_RED : TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, anyDry ? TFT_RED : TFT_DARKGREEN);
  tft.setTextSize(1);
  tft.setCursor(5, 220);
  tft.print(anyDry ? "WATERING IN PROGRESS" : "ALL PLANTS OK");
}

// -------------------- DRAW TEMP PAGE --------------------
void drawTempPage() {
  tft.fillScreen(TFT_BLACK);

  // Title
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(60, 4);
  tft.print("TEMPERATURE");

  drawDial(40,  130, 35, temp1, 50, "T1", "C", TFT_ORANGE);
  drawDial(120, 130, 35, temp2, 50, "T2", "C", TFT_ORANGE);
  drawDial(200, 130, 35, temp3, 50, "T3", "C", TFT_ORANGE);

  // Raw values below dials
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(5, 195);
  tft.print("T1:");tft.print(temp1,1);
  tft.print(" T2:");tft.print(temp2,1);
  tft.print(" T3:");tft.print(temp3,1);
  tft.print("C");

  // Status bar
  bool anyDry = (soil1 < threshold || soil2 < threshold || soil3 < threshold);
  tft.fillRect(0, 210, 240, 30, anyDry ? TFT_RED : TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, anyDry ? TFT_RED : TFT_DARKGREEN);
  tft.setTextSize(1);
  tft.setCursor(5, 220);
  tft.print(anyDry ? "WATERING IN PROGRESS" : "ALL PLANTS OK");
}

// -------------------- SERIAL OUTPUT --------------------
void printToSerial() {
  Serial.println("=========================");
  Serial.print("Soil 1: "); Serial.print(soil1); Serial.println("%");
  Serial.print("Soil 2: "); Serial.print(soil2); Serial.println("%");
  Serial.print("Soil 3: "); Serial.print(soil3); Serial.println("%");
  Serial.print("Temp 1: "); Serial.print(temp1, 1); Serial.println(" C");
  Serial.print("Temp 2: "); Serial.print(temp2, 1); Serial.println(" C");
  Serial.print("Temp 3: "); Serial.print(temp3, 1); Serial.println(" C");
  Serial.print("Pump 1: "); Serial.println(pump1 ? "ON" : "OFF");
  Serial.print("Pump 2: "); Serial.println(pump2 ? "ON" : "OFF");
  Serial.print("Pump 3: "); Serial.println(pump3 ? "ON" : "OFF");
  Serial.println("=========================");
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  analogReadResolution(14);

  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(PUMP3_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);

  stopAllPumps();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Boot screen
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 100);
  tft.print("Plant Kit");
  tft.setCursor(30, 130);
  tft.print("Starting...");
  delay(2000);

  Serial.println("Plant Watering System Starting...");
}

// -------------------- LOOP --------------------
void loop() {
  unsigned long now = millis();

  checkPumpTimers();

  // Switch page every 4 seconds
  if (now - lastPageSwitch >= PAGE_INTERVAL) {
    lastPageSwitch = now;
    currentPage = (currentPage + 1) % 2;
  }

  // Read sensors every 5 seconds
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;

    soil1 = readSoil(SOIL1_PIN);
    soil2 = readSoil(SOIL2_PIN);
    soil3 = readSoil(SOIL3_PIN);

    temp1 = readTemp(TEMP1_PIN);
    temp2 = readTemp(TEMP2_PIN);
    temp3 = readTemp(TEMP3_PIN);

    printToSerial();
    autoWater();
    updateLED();
  }

  // Draw current page
  if (currentPage == 0) drawSoilPage();
  else drawTempPage();

  delay(500);
}
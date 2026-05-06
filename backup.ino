#include <TFT_eSPI.h>
#include <TFT_eWidget.h>

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
MeterWidget soil1Meter = MeterWidget(&tft);
MeterWidget soil2Meter = MeterWidget(&tft);
MeterWidget soil3Meter = MeterWidget(&tft);
MeterWidget temp1Meter = MeterWidget(&tft);
MeterWidget temp2Meter = MeterWidget(&tft);
MeterWidget temp3Meter = MeterWidget(&tft);

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

// -------------------- METER SETUP --------------------
void setupSoilMeters() {
  soil1Meter.analogMeter(60, 0,   100, "SOIL 1", "0", "25", "50", "75", "100");
  soil2Meter.analogMeter(60, 80,  100, "SOIL 2", "0", "25", "50", "75", "100");
  soil3Meter.analogMeter(60, 160, 100, "SOIL 3", "0", "25", "50", "75", "100");
}

void setupTempMeters() {
  temp1Meter.analogMeter(60, 0,   50, "TEMP 1", "0", "12", "25", "37", "50");
  temp2Meter.analogMeter(60, 80,  50, "TEMP 2", "0", "12", "25", "37", "50");
  temp3Meter.analogMeter(60, 160, 50, "TEMP 3", "0", "12", "25", "37", "50");
}

// -------------------- PAGE TITLE --------------------
void drawPageTitle() {
  tft.fillRect(0, 0, 240, 28, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  tft.print(currentPage == 0 ? "Soil Moisture" : "Temperature");
}

// -------------------- STATUS BAR --------------------
void drawStatusBar() {
  bool anyDry = (soil1 < threshold || soil2 < threshold || soil3 < threshold);
  tft.fillRect(0, 210, 240, 30, anyDry ? TFT_RED : TFT_DARKGREEN);
  tft.setTextColor(TFT_WHITE, anyDry ? TFT_RED : TFT_DARKGREEN);
  tft.setTextSize(1);
  tft.setCursor(5, 220);
  if (anyDry) {
    tft.print("WATERING: ");
    if (pump1) tft.print("P1 ");
    if (pump2) tft.print("P2 ");
    if (pump3) tft.print("P3 ");
  } else {
    tft.print("ALL PLANTS OK");
  }
}

// -------------------- DISPLAY UPDATE --------------------
void updateDisplay() {
  drawPageTitle();
  if (currentPage == 0) {
    soil1Meter.updateNeedle(soil1, 0);
    soil2Meter.updateNeedle(soil2, 0);
    soil3Meter.updateNeedle(soil3, 0);
  } else {
    temp1Meter.updateNeedle(temp1, 0);
    temp2Meter.updateNeedle(temp2, 0);
    temp3Meter.updateNeedle(temp3, 0);
  }
  drawStatusBar();
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
  tft.setCursor(30, 100);
  tft.print("Plant Kit");
  tft.setCursor(20, 125);
  tft.print("Starting...");
  delay(2000);
  tft.fillScreen(TFT_BLACK);

  // Draw initial meters
  setupSoilMeters();

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
    tft.fillScreen(TFT_BLACK);
    if (currentPage == 0) setupSoilMeters();
    else setupTempMeters();
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

  updateDisplay();
}
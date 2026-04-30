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

int SOIL_DRY = 3400;
int SOIL_WET = 1800;

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

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 2000;

// -------------------- SENSOR FUNCTIONS --------------------
int readSoil(int pin) {
  int raw = analogRead(pin);
  int percent = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
  return constrain(percent, 0, 100);
}

float readTemp(int pin) {
  int raw = analogRead(pin);
  if (raw <= 0) return 0.0;
  float resistance = R0 * ((ADC_MAX / raw) - 1.0);
  float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(resistance / R0));
  return tempK - 273.15;
}

// -------------------- PUMP CONTROL --------------------
void setPump(int pin, bool &state, bool on) {
  digitalWrite(pin, on ? HIGH : LOW);
  state = on;
}

void stopAllPumps() {
  setPump(PUMP1_PIN, pump1, false);
  setPump(PUMP2_PIN, pump2, false);
  setPump(PUMP3_PIN, pump3, false);
}

// -------------------- AUTO WATER --------------------
void autoWater() {
  if (soil1 < threshold && !pump1) setPump(PUMP1_PIN, pump1, true);
  if (soil2 < threshold && !pump2) setPump(PUMP2_PIN, pump2, true);
  if (soil3 < threshold && !pump3) setPump(PUMP3_PIN, pump3, true);

  if (soil1 >= threshold && pump1) setPump(PUMP1_PIN, pump1, false);
  if (soil2 >= threshold && pump2) setPump(PUMP2_PIN, pump2, false);
  if (soil3 >= threshold && pump3) setPump(PUMP3_PIN, pump3, false);
}

// -------------------- LED STATUS --------------------
void updateLED() {
  bool anyDry = (soil1 < threshold || soil2 < threshold || soil3 < threshold);
  digitalWrite(RED_LED,   anyDry ? HIGH : LOW);
  digitalWrite(GREEN_LED, anyDry ? LOW  : HIGH);
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

  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(PUMP3_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);

  stopAllPumps();

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
}

// -------------------- LOOP --------------------
void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;

    soil1 = readSoil(SOIL1_PIN);
    soil2 = readSoil(SOIL2_PIN);
    soil3 = readSoil(SOIL3_PIN);

    temp1 = readTemp(TEMP1_PIN);
    temp2 = readTemp(TEMP2_PIN);
    temp3 = readTemp(TEMP3_PIN);

    autoWater();
    updateLED();
    printToSerial();
  }            
}
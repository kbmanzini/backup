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

bool pump1 = fa
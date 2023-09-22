#include <Arduino.h>
#include <Preferences.h>

extern Preferences prefs;

// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000  

struct {
  int gpioPin;
  float temperatureReadingNonCompensated;
  float compensationFactor;
} temperatureSensors[2] = {
  {
    .gpioPin=32,
    .temperatureReadingNonCompensated=NAN,
    .compensationFactor=NAN,
  },
  {
    .gpioPin=33,
    .temperatureReadingNonCompensated=NAN,
    .compensationFactor=NAN,
  }
};

float readTemperature(int gpioPin){
    //analogRead yields a 12bit number (0-4095)
  int voltageReading = analogRead(gpioPin);
  if(4095==voltageReading){
    return NAN;
  }

  //-30 celcius = 181.70KOhm
  //110 celcius = 0.5066kOhm
  float thermistorResistance = SERIESRESISTOR / (4095.0 / voltageReading - 1);

  float steinhart = thermistorResistance / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert absolute temp to C
  
  // Serial.printf("voltageReading=%u, thermistorResistance=%f, tempCelcius=%f\n", voltageReading, thermistorResistance, steinhart);
  return steinhart;
}

void readTemperatureFromSensors(){
  temperatureSensors[0].temperatureReadingNonCompensated=readTemperature(temperatureSensors[0].gpioPin);
  temperatureSensors[1].temperatureReadingNonCompensated=readTemperature(temperatureSensors[1].gpioPin);
}

#define TEMP_COMPENSATION_FACTOR0 "tempCompensationFactor0"
#define TEMP_COMPENSATION_FACTOR1 "tempCompensationFactor1"

void setKnownCompensationTemperatureTriac(float knownCurrentTempTriac){
  temperatureSensors[0].compensationFactor=knownCurrentTempTriac/temperatureSensors[0].temperatureReadingNonCompensated;
  Serial.printf("compensationFactor=%f\n", temperatureSensors[0].compensationFactor);
  prefs.putFloat(TEMP_COMPENSATION_FACTOR0, temperatureSensors[0].compensationFactor);
}

void initTemperature(){
  temperatureSensors[0].compensationFactor=prefs.getFloat(TEMP_COMPENSATION_FACTOR0, 1.0);
  Serial.printf("temperatureSensors[0].compensationFactor=%f\n", temperatureSensors[0].compensationFactor);
  temperatureSensors[1].compensationFactor=prefs.getFloat(TEMP_COMPENSATION_FACTOR1, 1.0);
  Serial.printf("temperatureSensors[1].compensationFactor=%f\n", temperatureSensors[1].compensationFactor);
}

void getTemperatureReadings(float& triacTemp, float& externalTemp){
  triacTemp=temperatureSensors[0].temperatureReadingNonCompensated*temperatureSensors[0].compensationFactor;
  externalTemp=temperatureSensors[1].temperatureReadingNonCompensated*temperatureSensors[1].compensationFactor;
}

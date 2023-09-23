#include <Arduino.h>
#include <Preferences.h>

extern Preferences prefs;

#define PREFKEY_TEMP_COMPENSATION_FACTOR0 "tempCompensationFactor0"
#define PREFKEY_TEMP_COMPENSATION_FACTOR1 "tempCompensationFactor1"

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
  float temperatureLimit;
} temperatureSensors[2] = {
  {
    .gpioPin=32,
    .temperatureReadingNonCompensated=NAN,
    .compensationFactor=NAN,
    .temperatureLimit=NAN,
  },
  {
    .gpioPin=33,
    .temperatureReadingNonCompensated=NAN,
    .compensationFactor=NAN,
    .temperatureLimit=NAN,
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


float getCompensatedTemperatureReading(int sensorIndex){
  return temperatureSensors[sensorIndex].temperatureReadingNonCompensated*temperatureSensors[sensorIndex].compensationFactor;
}

void getTemperatureReadings(float& triacTemp, float& externalTemp){
  triacTemp=getCompensatedTemperatureReading(0);
  externalTemp=getCompensatedTemperatureReading(1);
}

bool isTemperatureOverlimit(int sensorIndex){
  if(getCompensatedTemperatureReading(sensorIndex)>temperatureSensors[sensorIndex].temperatureLimit){
    return true;
  }
  return false;
}

void readTemperatureFromSensors(){
  bool needToCooldown=false;
  for(int i=0; i<2; i++){
    temperatureSensors[i].temperatureReadingNonCompensated=readTemperature(temperatureSensors[i].gpioPin);
    if(isTemperatureOverlimit(i)){
      needToCooldown=true;
    }
  }

  if(needToCooldown){
    
  }
}

void setKnownCompensationTemperatureTriac(float knownCurrentTempTriac){
  temperatureSensors[0].compensationFactor=knownCurrentTempTriac/temperatureSensors[0].temperatureReadingNonCompensated;
  Serial.printf("compensationFactor=%f\n", temperatureSensors[0].compensationFactor);
  prefs.putFloat(PREFKEY_TEMP_COMPENSATION_FACTOR0, temperatureSensors[0].compensationFactor);
}

void initTemperature(){
  temperatureSensors[0].compensationFactor=prefs.getFloat(PREFKEY_TEMP_COMPENSATION_FACTOR0, 1.0);
  Serial.printf("temperatureSensors[0].compensationFactor=%f\n", temperatureSensors[0].compensationFactor);
  temperatureSensors[1].compensationFactor=prefs.getFloat(PREFKEY_TEMP_COMPENSATION_FACTOR1, 1.0);
  Serial.printf("temperatureSensors[1].compensationFactor=%f\n", temperatureSensors[1].compensationFactor);
}

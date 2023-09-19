#include <Arduino.h>

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

float triacTemperatureSensor=NAN;
float externalTemperatureSensor=NAN;

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

void updateTemperatureReadings(){
  triacTemperatureSensor=readTemperature(32);
  externalTemperatureSensor=readTemperature(33);
}

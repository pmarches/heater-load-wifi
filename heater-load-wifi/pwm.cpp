#include "soc/rtc_wdt.h"
#include <Arduino.h>

const uint32_t PWM_PIN = 13;
const uint32_t AC_PHASE_DETECTION_PIN = 26;

QueueHandle_t phaseTSQueue;
uint32_t IRAM_ATTR highPulseTarget = 0;
uint32_t IRAM_ATTR lowPulseTarget = 0;
uint32_t targetWatts=0;

#define FULL_PERIOD_US (1000000 / 60)
#define HALF_PERIOD_US (1000000 / 60 / 2)
#define SYNC_CYCLE_COUNT 60  //60=Sync the waves once per second on 60Hz

unsigned long previousPhaseTS = 0;

void onACPhaseDetected() {
  unsigned long now = micros();
  if(0==previousPhaseTS){
    previousPhaseTS = now;
    return;
  }

  long period=now - previousPhaseTS;
  //Here we filter out many phase detections because we do not need so many to synchronize our pulseTrain. The pulsetrain is expected to take FULL_PERIOD_US*SYNC_CYCLE_COUNT
  if (period > (FULL_PERIOD_US * SYNC_CYCLE_COUNT) - 1000) {
    xQueueOverwriteFromISR(phaseTSQueue, &now, NULL);
    previousPhaseTS = now;
  }
}

void configurePhaseDetectQueue() {
  phaseTSQueue = xQueueCreate(1, sizeof(unsigned long));
  pinMode(AC_PHASE_DETECTION_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(AC_PHASE_DETECTION_PIN), onACPhaseDetected, FALLING);
}

//The total time taken by this function must not exceed FULL_PERIOD_US*SYNC_CYCLE_COUNT
void playPulseTrain() {
  if (0 == highPulseTarget) {
    digitalWrite(PWM_PIN, LOW);
    return;
  }
  if (0 == lowPulseTarget) {
    digitalWrite(PWM_PIN, HIGH);
    return;
  }

  //Copy the delays in local variables, since lowPulseTarget and highPulseTarget can change anytime. Probably need to protect lowPulseTarget with semaphore
  uint32_t lowDelay = lowPulseTarget * HALF_PERIOD_US;
  uint32_t highDelay = highPulseTarget * HALF_PERIOD_US;
  uint32_t pulseCount = (FULL_PERIOD_US * SYNC_CYCLE_COUNT) / (lowDelay + highDelay);
  for (uint32_t i = 0; i < pulseCount; i++) {
    digitalWrite(PWM_PIN, HIGH);
    delayMicroseconds(highDelay);

    digitalWrite(PWM_PIN, LOW);
    if (i + 1 == pulseCount) {
      break;
    }
    delayMicroseconds(lowDelay);
  }
}

void taskDoPulseTrainInSyncWithACPhase(void* args) {
  const unsigned long MAX_USEC_PHASE_START_TO_ZC = 3167;

  while (true) {
    unsigned long phaseTS;
    if (xQueueReceive(phaseTSQueue, &phaseTS, /*portMAX_DELAY*/ 1200/portTICK_PERIOD_MS) == pdFALSE) {
      Serial.println("Failed to receive phase timestamp from queue");
      continue;
    }
    unsigned long lagBetweenNowAndPhase = micros() - phaseTS;
    Serial.printf("lagBetweenNowAndPhase=%ld\n", lagBetweenNowAndPhase);
    if(lagBetweenNowAndPhase>MAX_USEC_PHASE_START_TO_ZC){
      Serial.printf("ERROR: lagBetweenNowAndPhase=%ld\n", lagBetweenNowAndPhase);
    }
    else{
      delayMicroseconds(MAX_USEC_PHASE_START_TO_ZC - lagBetweenNowAndPhase);
      //Ok, from here we are in sync and can control the pins however we want.
      playPulseTrain();
      Serial.println("Pulse train done");
    }
  }
}

void getPhaseSelectionCounts(unsigned int& highCount, unsigned int& lowCount) {
  highCount = highPulseTarget;
  lowCount = lowPulseTarget;
}

void setPhaseSelectionCounts(unsigned int highCount, unsigned int lowCount) {
  highPulseTarget = highCount;
  lowPulseTarget = lowCount;
}

void setTargetWatts(uint32_t targetWatts, uint32_t loadWatts){
  ::targetWatts=targetWatts;

  double BSFdenominator=60;
  double BSFdecimal=1.0;
  double duty=(targetWatts*1.0)/(loadWatts*1.0);
  //Goal here is to convert the duty into a fraction with the smallest denominator. We can use a fraction that is of a lower value than the duty. The denominator cannot be larger than 60 (hz)
  //TODO This could be improved by iterating over prime denominators
  for(int i=59; i>0; i--){
    double integerPart;
    double decimalPart=modf(duty*i, &integerPart);
    // Serial.printf("i=%d integerPart=%f decimalPart=%f BSFdenominator=%f\n", i, integerPart, decimalPart, BSFdenominator);
    if(decimalPart <= BSFdecimal){
      BSFdecimal=decimalPart;
      BSFdenominator=i;
    }
  }
  int32_t onCount=duty*BSFdenominator;
  int32_t offCount=BSFdenominator-onCount;
  Serial.printf("Computed onCount=%d offCount=%d\n", onCount, offCount);
  setPhaseSelectionCounts(onCount, offCount);
}

uint32_t getTargetWatts(){
  return ::targetWatts;
}

void initPWM() {
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  configurePhaseDetectQueue();
  rtc_wdt_set_time(RTC_WDT_STAGE0, 20000);
  // rtc_wdt_disable();
  uint32_t priority = 1;
  xTaskCreatePinnedToCore(taskDoPulseTrainInSyncWithACPhase, "taskDoPulseTrainInSyncWithACPhase", 2000, NULL, priority, NULL, 1);
}

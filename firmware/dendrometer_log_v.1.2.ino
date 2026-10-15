#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h>
#include "LowPower.h"

const uint8_t RTC_INT_PIN = 3;               // DS3231 SQW/INT -> D3
const TimeSpan LOG_INTERVAL(0, 0, 10, 0);    // days, hours, minutes, seconds (10 min)

Adafruit_ADS1115 ads;
RTC_DS3231 rtc;

void wakeup() {
  detachInterrupt(digitalPinToInterrupt(RTC_INT_PIN));
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(RTC_INT_PIN, INPUT_PULLUP);

  rtc.begin();
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));   // uncomment once to set the clock
  rtc.writeSqwPinMode(DS3231_OFF);   // alarm interrupt mode (required for setAlarm1)
  rtc.disableAlarm(2);
  rtc.clearAlarm(2);

  SD.begin();

  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 not found!");
    while (1) {}
  }
  ads.setGain(GAIN_ONE);
}

void loop() {
  DateTime now = rtc.now();
  int16_t raw = ads.readADC_SingleEnded(0);
  float v = ads.computeVolts(raw);

  Serial.print(now.timestamp(DateTime::TIMESTAMP_FULL));
  Serial.print(",");
  Serial.print(raw);
  Serial.print(",");
  Serial.println(v, 4);

  File dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    if (dataFile.size() == 0) {
      dataFile.println("timestamp,sensor,voltage");
    }
    dataFile.print(now.timestamp(DateTime::TIMESTAMP_FULL));
    dataFile.print(",");
    dataFile.print(raw);
    dataFile.print(",");
    dataFile.println(v, 4);
    dataFile.close();
  }

  // Clear the old alarm first (releases INT pin), then set the next one
  rtc.clearAlarm(1);
  rtc.setAlarm1(rtc.now() + LOG_INTERVAL, DS3231_A1_Minute);

  Serial.flush();   // finish sending serial data before sleeping
  attachInterrupt(digitalPinToInterrupt(RTC_INT_PIN), wakeup, FALLING);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
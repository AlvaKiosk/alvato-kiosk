#include "config.h"
#include "kioskControl.h"
#include <Arduino.h>
#include <Preferences.h>




void initGPIO(unsigned long long INP, unsigned long long OUTP){
  gpio_config_t io_config;

  Serial.printf("  Execute---Initial GPIO Function\n");
  //*** Initial INTERRUPT PIN
//   io_config.intr_type = GPIO_INTR_NEGEDGE;
//   io_config.pin_bit_mask = INTR;
//   io_config.mode = GPIO_MODE_INPUT;
//   io_config.pull_up_en = GPIO_PULLUP_ENABLE;
//   gpio_config(&io_config);
  
  //*** Initial INPUT PIN
  io_config.pin_bit_mask = INP;
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_config);


  //*** Initial INPUT & OUTPUT PIN
  io_config.pin_bit_mask = OUTP;
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT_OUTPUT;
  gpio_config(&io_config);
}



void printLocalTime(tm * timeinfo){
  // struct tm timeinfo;
  if (!getLocalTime(timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(timeinfo, "%A, %B %d %Y %H:%M:%S");

  // char buff[5];
  // strftime(buff,5,"%Y",&timeinfo);
  // Serial.println(buff);

  // strftime(buff,5,"%d",&timeinfo);
  // Serial.println(buff);
}
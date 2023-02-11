#include <stdio.h>
#include <math.h>
#include "esp_system.h"
#include "esp_attr.h"
#include "Arduino.h"
/*ESP32 IDF*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"
#include "driver/adc.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "freertos/semphr.h"
#include "driver/dac.h"
/*ARDUINO*/
#include "Wire.h"
#include "Adafruit_INA219.h"

Adafruit_INA219 ina219;
mcpwm_config_t pwm_config;



#define GPIO1 19  // pra IN1 Ponte H
#define GPIO2 16 // pra IN2 Ponte H

int freq_switch = 1000;

float readINA10k (){
  float data = 0;
  data = ina219.getCurrent_mA();
  delayMicroseconds(100);
  return data;
}

void setup()
{
  Serial.begin(9600);
  /*MCPWM*/
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO1);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO2); 
  pwm_config.frequency = freq_switch;    
  pwm_config.cmpr_a = 100;    		
  pwm_config.cmpr_b = 0;    		
  pwm_config.counter_mode = MCPWM_UP_DOWN_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   
  mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE,20 ,20 );  //Enable deadtime on PWM0A and PWM0B with red = (20)*100ns & fed = (20)*100ns on PWM0A and PWM0B generated from PWM0A

  /*ADC*/  
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  adc1_config_width(ADC_WIDTH_BIT_10);
  
  /*SENSOR*/
  Serial.begin(9600);
  while (!Serial) {;} 
    if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
}

void loop()
{
    float iin=0;
    for( int i=0; i<5 ; i++)
      iin += readINA10k();
    iin /= 5;
    int iout = (int)(iin); //        100/40 
    Serial.println(iout);
    unsigned int ref = (int)(adc1_get_raw(ADC1_CHANNEL_6)*100/1023);
    ref = (int)(adc1_get_raw(ADC1_CHANNEL_6)*100/1023);
	  //Serial.println(ref); 
    if (ref>50)
    {
      //int err = ref - iout;
      //if(err>0 && err<=100){
      // pwm_config.cmpr_a = err; 
      // pwm_config.cmpr_b = 100 - err; 
      // mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);  
      // Serial.println(iout);
  }
  vTaskDelay(1);

}


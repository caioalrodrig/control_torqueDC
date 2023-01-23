#include <stdio.h>
#include "esp_system.h"
#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "driver/timer.h"
#include "freertos/semphr.h"
#include "driver/adc.h"

#define TIMER_DIVIDER   (16)
#define GPIO1 19  // pra IN1 Ponte H
#define GPIO2 16 // pra IN2 Ponte H

static SemaphoreHandle_t timer_sem;
mcpwm_config_t pwm_config;
int freq_switch = 10000;
//control var
int num[4] ={0.02427 , - 0.0231 , - 0.02427 , 0.0231};
int den[4] ={1, - 2.999 , 2.998, - 0.999};
int buffer_adc[5] = {0,0,0,0,0};
int buffer_pwm[5] ={0,0,0,0,0};
int pwm_atual=0;

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{

    BaseType_t high_task_awoken = pdFALSE;	
    /*leitura da corrente, subtrai pelo valor de pwm atual, e depois faz a logica*/
    buffer_adc[0] = adc1_get_raw(ADC1_CHANNEL_6) - pwm_atual;
    buffer_pwm[0]=  (num[0]*buffer_adc[0]+ num[1]*buffer_adc[1]+ num[2]*buffer_adc[2]+ num[3]*buffer_adc[3]-den[1]*buffer_pwm[1]- den[2]*buffer_pwm[2]- den[3]*buffer_pwm[3])/den[0];
    /*deslocamento dos buffer pra direita pra liberar o indice 0 */
    for(int i=4;i>0;i--)
    {
        buffer_adc[i]=buffer_adc[i-1];
        buffer_pwm[i]=buffer_pwm[i-1];
    }
    /*escreve nos pwms o valor novo de dutycicle*/
    pwm_atual = (int)buffer_pwm[0];
    pwm_config.cmpr_a = pwm_atual;    		
    pwm_config.cmpr_b = 100 - pwm_atual;    

    xSemaphoreGiveFromISR(timer_sem, &high_task_awoken);
    return (high_task_awoken == pdTRUE); 
}

void app_main()
{
    
    timer_sem = xSemaphoreCreateBinary();
    if (timer_sem == NULL) 
        printf("Binary semaphore can not be created");

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO1);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO2); 

    pwm_config.frequency = freq_switch;    
    pwm_config.cmpr_a = 40;    		
    pwm_config.cmpr_b = 60;    		
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;


    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);   
    mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 0, 0);  //Enable deadtime on PWM0A and PWM0B with red = (656)*100ns & fed = (67)*100ns on PWM0A and PWM0B generated from PWM0A


    //timer configs ... intervalo, up, if stars in 1 or 0, isr on, reload on 
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_EN
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 1.0/(freq_switch)*(TIMER_BASE_CLK / TIMER_DIVIDER));
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_group_isr_callback, NULL, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_10);
;

    while(1){
        if (xSemaphoreTake(timer_sem, portMAX_DELAY) == pdPASS) 
        {
            
        }
    }
}


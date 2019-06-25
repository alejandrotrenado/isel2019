#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"
#define LED 2
#define PERIOD_TICK 100/portTICK_RATE_MS
#define tiempoGuarda 120/portTICK_RATE_MS
#define segundo 1000/portTICK_RATE_MS

volatile int timeout_1 = 0;
volatile int timeout_2 = 0;
int preparado=0;

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/

enum fsm_state {
  LED_ON,
  LED_OFF,
  ARMADO,
  DESARMADO,
};


uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/*funciones de comprobación */

int detec_presencia_1 (fsm_t *this) {
    if(!GPIO_INPUT_GET(12) && preparado==1){
    if (xTaskGetTickCount () > timeout_1) {
  		timeout_1 = xTaskGetTickCount () + tiempoGuarda ;
  		return 1;
  	}
    return 0;
  }
  return 0;

}

int detec_presencia_2 (fsm_t *this) {
    if(GPIO_INPUT_GET(12) || preparado==0){
    if (xTaskGetTickCount () > timeout_1) {
  		timeout_1 = xTaskGetTickCount () + tiempoGuarda ;
  		return 1;
  	}
    return 0;
  }
  return 0;

}

int comp_armar_1 (fsm_t *this) {
    if(!GPIO_INPUT_GET(14)){
    if (xTaskGetTickCount () > timeout_2) {
  		timeout_2 = xTaskGetTickCount () + tiempoGuarda ;
  		return 1;
  	}
    return 0;
  }
  return 0;

}

int comp_armar_2 (fsm_t *this) {
    if(GPIO_INPUT_GET(14)){
    if (xTaskGetTickCount () > timeout_2) {
  		timeout_2 = xTaskGetTickCount () + tiempoGuarda ;
  		return 1;
  	}
    return 0;
  }
  return 0;

}

/* funciones de salida */

void led_off (fsm_t *this) {
  GPIO_OUTPUT_SET(LED, 1);
}

void led_on (fsm_t *this) {

  GPIO_OUTPUT_SET(LED, 0);
}

void armar (fsm_t *this){
    preparado=1;

}
void desarmar (fsm_t *this){
    preparado=0;

}
/*
 * Máquina de estados: lista de transiciones
 * { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
 */
static fsm_trans_t detector_presencia[] = {
  { LED_OFF, detec_presencia_1, LED_ON,  led_on },
  { LED_ON,  detec_presencia_2, LED_OFF, led_off},
  {-1, NULL, -1, NULL },
};

static fsm_trans_t armar_alarma[] = {
  { DESARMADO, comp_armar_1, ARMADO,  armar },
  { ARMADO,  comp_armar_2, DESARMADO, desarmar},
  {-1, NULL, -1, NULL },
};


void sensor(void* ignore)
{   
    fsm_t* fsm_1 = fsm_new(detector_presencia);
    fsm_t* fsm_2 = fsm_new(armar_alarma);
    portTickType xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount ();
    while(true) {
      fsm_fire(fsm_1);
      fsm_fire(fsm_2);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }

    
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    PIN_FUNC_SELECT(GPIO_PIN_REG_12,FUNC_GPIO12);
    PIN_FUNC_SELECT(GPIO_PIN_REG_14,FUNC_GPIO14);
    xTaskCreate(&sensor, "startup", 2048, NULL, 1, NULL);
}


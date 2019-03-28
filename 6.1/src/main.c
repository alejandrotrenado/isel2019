#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"


#define LED 2
volatile int timeout = 0;
#define minuto 6000/portTICK_RATE_MS
#define tiempoGuarda 120/portTICK_RATE_MS


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

#define PERIOD_TICK 100/portTICK_RATE_MS
enum fsm_state {
  ARMADO,
  DESARMADO,
};
/* si el boton del GPIO 14,D5 est� pulsada entonces quitaremos los rebotes
y armaremos la alarma*/
int armar (fsm_t *this) {
    if(!GPIO_INPUT_GET(14)){
    if (xTaskGetTickCount () < timeout) {
  		timeout = xTaskGetTickCount () + tiempoGuarda ;
  		return 0;
  	}
  	timeout = xTaskGetTickCount () + tiempoGuarda ;
    return 1;
  }
  return 0;

}
/* si el boton del GPIO 14,D5 no est� pulsada entonces quitaremos los rebotes
y desarmaremos la alarma*/
int desarmar (fsm_t *this) {
    if(GPIO_INPUT_GET(14)){
    if (xTaskGetTickCount () < timeout) {
  		timeout = xTaskGetTickCount () + tiempoGuarda ;
  		return 0;
  	}
  	timeout = xTaskGetTickCount () + tiempoGuarda ;
    return 1;
  }
  return 0;

}
/* si una vez armada la alarma se detecta presencia, activaremos el led*/
int detec_presencia (fsm_t *this) {
    if(GPIO_INPUT_GET(12)){
    if (xTaskGetTickCount () < timeout) {
  		timeout = xTaskGetTickCount () + tiempoGuarda ;
  		return 0;
  	}
  	timeout = xTaskGetTickCount () + tiempoGuarda ;
    return 1;
  }
  return 0;

}
void led_off (fsm_t *this) {
  GPIO_OUTPUT_SET(LED, 1);
}

void led_on (fsm_t *this) {
  GPIO_OUTPUT_SET(LED, 0);
}

/*
 * M�quina de estados: lista de transiciones
 * { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
 */
static fsm_trans_t interruptor[] = {
  { ARMADO, desarmar, DESARMADO,  led_off },
  { DESARMADO,  armar, ARMADO, led_off},
  { ARMADO,  detec_presencia, ARMADO, led_on},
  {-1, NULL, -1, NULL },
};

void inter(void* ignore)
{
    fsm_t* fsm = fsm_new(interruptor);
    led_on(fsm);
    led_off(fsm);
    portTickType xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount ();
    while(true) {
      fsm_fire(fsm);
      vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
    }
}

void user_init(void)
{
    xTaskCreate (inter, "startup", 2048, NULL, 1, NULL);
}
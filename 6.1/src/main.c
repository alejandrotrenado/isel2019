#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"


#define LED 2
volatile int timeout = 0;
#define minuto 6000/portTICK_RATE_MS
#define tiempoGuarda 200/portTICK_RATE_MS


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
/* si el boton del GPIO 0,D3 está pulsada entonces quitaremos los rebotes
y encenderemos el led*/
int detec_presencia (fsm_t *this) {
  static portTickType xLastISRTick1 = 0;
  portTickType now = xTaskGetTickCount ();
  //GPIO_OUTPUT_SET(LED,GPIO_INPUT_GET(15));
  if( GPIO_INPUT_GET(13)&&!GPIO_INPUT_GET(0)){
    //GPIO_OUTPUT_SET(LED,0); 
    if(now>xLastISRTick1){
      return 1;
    }
    xLastISRTick1 = now + tiempoGuarda;
  return 0;
  }
  else{
  return 0;}
}
/* si el boton del GPIO 14,D5 está pulsada entonces quitaremos los rebotes
y armaremos la alarma*/
int armar (fsm_t *this) {
  static portTickType xLastISRTick0 = 0;
  portTickType now = xTaskGetTickCount ();
  if( !GPIO_INPUT_GET(0)){
    if(now>xLastISRTick0){
      return 1;
    }
    xLastISRTick0 = now + tiempoGuarda;
  return 0;
  }
  else{
  return 0;}
}
/* si el boton del GPIO 14,D5 no está pulsado entonces quitaremos los rebotes
y desarmaremos la alarma*/
int desarmar (fsm_t *this) {
  static portTickType xLastISRTick0 = 0;
  portTickType now = xTaskGetTickCount ();
  if( GPIO_INPUT_GET(0)){
    if(now>xLastISRTick0){
      return 1;
    }
    xLastISRTick0 = now + tiempoGuarda;
  return 0;
  }
  else{
  return 0;}
}

void led_on (fsm_t *this) {
GPIO_OUTPUT_SET(LED,0);
}
void led_off (fsm_t *this) {
GPIO_OUTPUT_SET(LED,1);
}

/*
 * Máquina de estados: lista de transiciones
 * { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
 */
static fsm_trans_t interruptor[] = {
  {DESARMADO, armar, ARMADO, led_off },
  {ARMADO, detec_presencia, ARMADO, led_on},
  {ARMADO, desarmar, DESARMADO, led_off},
  {-1, NULL, -1, NULL },
};

void inter(void* ignore)
{
    fsm_t* fsm = fsm_new(interruptor);
    //led_on(fsm);
    //led_off(fsm);
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

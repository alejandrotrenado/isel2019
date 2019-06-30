#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "stdio.h"
#include "fsm.h"
#define LED 2
#define PERIOD_TICK 100/portTICK_RATE_MS
#define tiempoGuarda 100/portTICK_RATE_MS
#define segundo 1000/portTICK_RATE_MS

volatile int timeout = 0;
int cuenta=0;
int timeout_seg=0;
int correcto=0;
int CONTR_0=1;
int CONTR_1=1;
int CONTR_2=1;
int aux=0;
volatile int tiempo_apagar=0;


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
IDLE,
CIFRA0,
CIFRA1,
CIFRA2,
APAGADO,
ENCENDIDO,
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

int cont_correcta(){
  if(correcto==1){
    if(!GPIO_INPUT_GET(0)){
    if (xTaskGetTickCount () > timeout) {
  		timeout = xTaskGetTickCount () + tiempoGuarda ;
  		return 1;
  	}
    return 0;
  }}

  return 0;

}
int cont_incorrecta(){
  if(correcto==0){
    return 1;
  }
  return 0;
}

int tocar(fsm_t *this){
    
    if(!GPIO_INPUT_GET(12) ){
    if (xTaskGetTickCount () > timeout) {
  		timeout = xTaskGetTickCount () + tiempoGuarda ;
  		return 1;
  	}
    return 0;
  }
  return 0;

}

int timeout_1s_button(){
  if(xTaskGetTickCount()>=timeout_seg){
    GPIO_OUTPUT_SET(LED, 0);
    vTaskDelay(10);
    GPIO_OUTPUT_SET(LED, 1);
    return 1;
  }
  if(!GPIO_INPUT_GET(12) ){
    if (xTaskGetTickCount () > timeout) {
      timeout = xTaskGetTickCount () + tiempoGuarda ;
      timeout_seg=xTaskGetTickCount()+segundo;
  		cuenta++;
  	}
  }

return 0;
}

int timeout_1s(fsm_t *this){
    if (xTaskGetTickCount () > tiempo_apagar) {
      return 1;
  	}
    return 0;

}

/* funciones de salida */

void inicializar(){
  aux=0;
  cuenta=0;
  correcto=0;
  timeout_seg=xTaskGetTickCount()+segundo;
  GPIO_OUTPUT_SET(LED, 1);
}
void comprobar_cifra_0(){
  if(cuenta==10){
    cuenta=0;
  }
  if (cuenta== CONTR_0){
    aux=1;
  }
  else{
    aux=0;
  }
  cuenta=0;
  timeout_seg=xTaskGetTickCount()+segundo;
}
void comprobar_cifra_1(){
  if(cuenta==10){
    cuenta=0;
  }
  if(cuenta!= CONTR_1 || aux==0){
    aux=0;
  }
  cuenta=0;
  timeout_seg=xTaskGetTickCount()+segundo;  
}
void comprobar_cifra_2(){
  if(cuenta==10){
    cuenta=0;
  }
  if(cuenta!= CONTR_2 || aux==0){
    aux=0;
  }
  if(aux==1){
    correcto=1;
  }
  cuenta=0;  
  timeout_seg=xTaskGetTickCount()+segundo;
  
}

void LED_ON(){
  tiempo_apagar=xTaskGetTickCount()+segundo;
  GPIO_OUTPUT_SET(LED, 0);
  
}

void LED_OFF(){
  GPIO_OUTPUT_SET(LED, 1);

}
/*
 * Máquina de estados: lista de transiciones
 * { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
 */
static fsm_trans_t cifrado[] = {
  { IDLE, tocar , CIFRA0,  inicializar },
  { CIFRA0, timeout_1s_button , CIFRA1,  comprobar_cifra_0 },
  { CIFRA1, timeout_1s_button , CIFRA2,  comprobar_cifra_1 },
  { CIFRA2, timeout_1s_button , IDLE,  comprobar_cifra_2 },
  {-1, NULL, -1, NULL },
};
static fsm_trans_t encendido[]={
  {APAGADO,cont_correcta,ENCENDIDO,LED_ON},
  {ENCENDIDO,timeout_1s,APAGADO,LED_OFF},
  {-1,NULL,-1,NULL},
};



/*/void sensor(void* ignore)
{   
    fsm_t* fsm_1 = fsm_new(cifrado);
    fsm_t* fsm_2 = fsm_new(encendido);
    portTickType xLastWakeTime;
    portTickType period= 20/portTICK_RATE_MS;
    xLastWakeTime = xTaskGetTickCount ();
    static int frame= 0;
    while(true) {    
      switch (frame){
        case 0 : fsm_fire(fsm_2);break;
        case 1 : fsm_fire(fsm_1);fsm_fire(fsm_2);break;
      }
      vTaskDelayUntil(&xLastWakeTime, period);
      frame=(frame+1)%2;
    }

    
}
*/

static void presencia(){
  fsm_t* fsm_2 = fsm_new(encendido);
  portTickType xLastWakeTime;
  portTickType period= 20/portTICK_RATE_MS;
  xLastWakeTime = xTaskGetTickCount ();
  while (1) {
    fsm_fire(fsm_2);
    vTaskDelayUntil (&xLastWakeTime, period);
  }

}
static void int_contra(){
  fsm_t* fsm_1 = fsm_new(cifrado);
  portTickType xLastWakeTime;
  portTickType period= 40/portTICK_RATE_MS;
  xLastWakeTime = xTaskGetTickCount ();
  while (1) {
    fsm_fire(fsm_1);
    vTaskDelayUntil (&xLastWakeTime, period);
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
    xTaskHandle task;
    xTaskCreate(&int_contra, "startup", 2048, NULL, 1, &task);
    xTaskCreate(&presencia, "startup", 2048, NULL, 2, &task);
}


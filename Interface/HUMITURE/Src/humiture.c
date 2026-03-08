#include "humiture.h"
#include "oled.h"
#include "dht11.h"
#include "usart.h"
#include "stdio.h"
int8_t temp = 0, humid = 0;

void humiture_init(void){

  Inf_DHT11_Init();
}


void humiture_read(void){
  Inf_DHT11_getData(&temp, &humid);
  printf("ÎÂ¶È£º%d, Êª¶È£º%d\r\n", temp, humid);

  OLED_Show_humiture(temp, humid);



}

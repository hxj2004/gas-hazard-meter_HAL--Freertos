#include "oled.h"

void OLED_Init(void){
  SSD1315_Init();  //初始化OLED

  OLED_Fill_AllOff(); // 关闭OLED显示，清屏

  // 在OLED第0页、第0列显示16*16“中”字
  //污染气体
  OLED_ShowHz_12x12(0,0,0); 
  OLED_ShowHz_12x12(12,0,1); 

  OLED_ShowNum_12x6(24,0,26); 
  OLED_ShowNum_12x6(60,0,80);
  OLED_ShowNum_12x6(66,0,80);
  OLED_ShowNum_12x6(72,0,77);

  // //可燃
  OLED_ShowHz_12x12(0,16,2); 
  OLED_ShowHz_12x12(12,16,3); 

  OLED_ShowNum_12x6(24,16,26); 
  OLED_ShowNum_12x6(60,16,80);
  OLED_ShowNum_12x6(66,16,80);
  OLED_ShowNum_12x6(72,16,77);

  // //甲醛
  OLED_ShowHz_12x12(0,32,4); 
  OLED_ShowHz_12x12(12,32,5); 

  OLED_ShowNum_12x6(24,32,26); 
  OLED_ShowNum_12x6(109,32,80);
  OLED_ShowNum_12x6(115,32,80);
  OLED_ShowNum_12x6(121,32,77);
  // //二氧化碳
  OLED_ShowHz_12x12(0,48,6); 
  OLED_ShowHz_12x12(12,48,7); 
  OLED_ShowHz_12x12(24,48,8); 
  OLED_ShowHz_12x12(36,48,9); 

  OLED_ShowNum_12x6(49,48,26); 
  OLED_ShowNum_12x6(109,48,80);
  OLED_ShowNum_12x6(115,48,80);
  OLED_ShowNum_12x6(121,48,77);
  // //温度
  OLED_ShowHz_12x12(78,0,10); 
  OLED_ShowHz_12x12(90,0,11); 


  OLED_ShowNum_12x6(121,0,96);



  // // //湿度
  OLED_ShowHz_12x12(78,16,12); 
  OLED_ShowHz_12x12(90,16,11); 


  OLED_ShowNum_12x6(121,16,5);
}

void OLED_Show_humiture(int8_t temp,int8_t humid){
  OLED_ClearArea(103,0,18,32);
  if (temp < 0)
  {
    OLED_ShowNum_12x6(103,0,13);
    
  }
  OLED_ShowNum_12x6(109,0,temp/10+16);
  OLED_ShowNum_12x6(115,0,temp%10+16);

  OLED_ShowNum_12x6(109,16,humid/10+16);
  OLED_ShowNum_12x6(115,16,humid%10+16);
  
  

}


uint8_t  pos = 0;

void OLED_Show_mq2(uint16_t mq2_value){
  OLED_ClearArea(30,16,30,12);
  if (mq2_value / 10000 > 0)
  {
    OLED_ShowNum_12x6(30 + pos++ *6,16,(mq2_value/10000)+16);
  }
   if (mq2_value / 1000 > 0)
  {
    OLED_ShowNum_12x6(30 + pos++ *6,16,((mq2_value/1000)%10)+16);
  }
  if (mq2_value / 100 > 0)
  {
    OLED_ShowNum_12x6(30 + pos++ *6,16,((mq2_value/100)%10)+16);
  }
   if (mq2_value / 10 > 0)
  {
    OLED_ShowNum_12x6(30 + pos++ *6,16,((mq2_value/10)%10)+16);
  }

  OLED_ShowNum_12x6(30 + pos++ *6,16,(mq2_value%10)+16);
  
  
  pos = 0;
  
  
}

void OLED_Show_voc201(float tvoc,float ch2o,uint16_t co2){
   OLED_ClearArea(30,  0, 30, 12);
   OLED_ClearArea(30, 32, 30, 12);
   OLED_ClearArea(54, 48, 30, 12);

    
    // ================= TVOC 显示 (1位整数.3位小数) =================
    // 假设 font 库中：'0'的偏移是16，那么'.'的偏移通常是 14 (ASCII码 '.'=46, '0'=48)
    // 如果你的屏幕显示出错误的符号，请修改下面的 DOT_INDEX
    const uint8_t DOT_INDEX = 14; 

    // 1. 整数位 (个位)
    OLED_ShowNum_12x6(30 + pos++ * 6, 0, ((int)tvoc % 10) + 16);

    // 2. 小数点
    OLED_ShowNum_12x6(30 + pos++ * 6, 0, DOT_INDEX);

    // 3. 小数第1位 (十分位)
    OLED_ShowNum_12x6(30 + pos++ * 6, 0, ((int)(tvoc * 10) % 10) + 16);

    // 4. 小数第2位 (百分位)
    OLED_ShowNum_12x6(30 + pos++ * 6, 0, ((int)(tvoc * 100) % 10) + 16);

    // 5. 小数第3位 (千分位)
    OLED_ShowNum_12x6(30 + pos++ * 6, 0, ((int)(tvoc * 1000) % 10) + 16);


    // ================= 甲醛 CH2O 显示 (1位整数.3位小数) =================
    pos = 0; // 重置光标位置
    
    // 1. 整数位
    OLED_ShowNum_12x6(30 + pos++ * 6, 32, ((int)ch2o % 10) + 16);

    // 2. 小数点
    OLED_ShowNum_12x6(30 + pos++ * 6, 32, DOT_INDEX);

    // 3. 小数第1位
    OLED_ShowNum_12x6(30 + pos++ * 6, 32, ((int)(ch2o * 10) % 10) + 16);

    // 4. 小数第2位
    OLED_ShowNum_12x6(30 + pos++ * 6, 32, ((int)(ch2o * 100) % 10) + 16);

    // 5. 小数第3位
    OLED_ShowNum_12x6(30 + pos++ * 6, 32, ((int)(ch2o * 1000) % 10) + 16);

  
  
  pos = 0;


  if (co2 / 10000 > 0)
  {
    OLED_ShowNum_12x6(54 + pos++ *6,48,(co2/10000)+16);
  }
   if (co2 / 1000 > 0)
  {
    OLED_ShowNum_12x6(54 + pos++ *6,48,((co2/1000)%10)+16);
  }
  if (co2 / 100 > 0)
  {
    OLED_ShowNum_12x6(54 + pos++ *6,48,((co2/100)%10)+16);
  }
   if (co2 / 10 > 0)
  {
    OLED_ShowNum_12x6(54 + pos++ *6,48,((co2/10)%10)+16);
  }

  OLED_ShowNum_12x6(54 + pos++ *6,48,(co2%10)+16);
  
  
  pos = 0;
   

}





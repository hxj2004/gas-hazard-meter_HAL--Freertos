#ifndef __SSD1315_H__
#define __SSD1315_H__

#include "stm32f1xx_hal.h"
#include "font.h"
#include "i2c.h"
void SSD1315_Init(void);
void OLED_SendCmd(uint8_t cmd);
// 向OLED发送1个数据（像素数据）
void OLED_SendData(uint8_t data);
// 全屏点亮（验证OLED是否可用）
void OLED_FillScreen(void);
// 全屏熄灭（验证OLED是否可用）
void OLED_Fill_AllOff(void);   // 全屏清空（所有像素灭，清屏用）
// 设置OLED显示区域（列页）
void OLED_SetArea(uint8_t start_col, uint8_t end_col, uint8_t start_page, uint8_t end_page);
// 显示字符16x16


// 显示字符12x6
void OLED_ShowNum_12x6(uint8_t x, uint8_t y, uint8_t num);

// 显示汉字12x12
void OLED_ShowHz_12x12(uint8_t x, uint8_t y, uint8_t num);

void OLED_ClearArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

#endif /* __SSD1315_H__ */


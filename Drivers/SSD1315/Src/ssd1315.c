#include "ssd1315.h"

#define OLED_I2C_ADDR 0x78

//初始化OLED屏幕
void SSD1315_Init(void)
{
  HAL_Delay(100); // 等待电源稳定

  // 初始化命令序列（按“关显示→硬件配置→电源配置→地址配置→开显示”顺序）
  OLED_SendCmd(0xAE); // 1. 关闭显示（避免配置时乱显）

  // 2. 硬件参数配置（适配128×64屏幕）
  OLED_SendCmd(0xA8); // 多路复用率配置
  OLED_SendCmd(0x3F); // 64 COM 模式（必须是0x3F，对应64行）
  OLED_SendCmd(0xD3); // 显示偏移
  OLED_SendCmd(0x00); // 偏移0（默认）
  OLED_SendCmd(0x40); // 显示起始行=0
  OLED_SendCmd(0xA1); // 列重映射（0xA1=正常，0xA0=反转）
  OLED_SendCmd(0xC8); // COM 扫描方向（0xC8=正常，0xC0=反转）
  OLED_SendCmd(0xDA); // COM 引脚配置
  OLED_SendCmd(0x12); // 64 COM 交替驱动（必须是0x12）

  // 3. 电源与显示参数配置
  OLED_SendCmd(0xD5); // 振荡器频率
  OLED_SendCmd(0x80); // 默认最优值
  OLED_SendCmd(0xD9); // 预充电周期（3.3V供电用0xF1）
  OLED_SendCmd(0xF1);
  OLED_SendCmd(0xDB); // VCOMH 电平配置
  OLED_SendCmd(0x40); // 默认值（合适亮度）
  OLED_SendCmd(0x8D); // 电荷泵配置（核心！）
  OLED_SendCmd(0x14); // 使能电荷泵（必须开）

  // 【关键补充：地址模式和地址范围配置】
  OLED_SendCmd(0x20); // 设置显存地址模式
  OLED_SendCmd(0x00); // 页模式（0x02，和你的 FillScreen 逻辑匹配）
  OLED_SendCmd(0x21); // 设置列地址范围
  OLED_SendCmd(0x00); // 起始列=0
  OLED_SendCmd(0x7F); // 结束列=127（全屏宽度）
  OLED_SendCmd(0x22); // 设置页地址范围
  OLED_SendCmd(0x00); // 起始页=0
  OLED_SendCmd(0x07); // 结束页=7（全屏高度，8页×8行=64行）

  // 4. 开启显示
  OLED_SendCmd(0xA6); // 正向显示（1=亮，0=灭）
  OLED_SendCmd(0xAF); // 开启显示
}

// 向OLED发送1个命令（逻辑不变，正确）
void OLED_SendCmd(uint8_t cmd)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);
}

// 向OLED发送1个数据（逻辑不变，正确）
void OLED_SendData(uint8_t data)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// 批量发送数据（新增：提升多字节传输效率，替代循环调用OLED_SendData）
void OLED_SendData_Batch(uint8_t *data, uint16_t len)
{
  if (len == 0 || data == NULL)
    return;
  HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

// 全屏点亮（逻辑不变，循环8页正确，修改后会执行完整）
void OLED_FillScreen(void)
{
  uint8_t page, col;

  for (page = 0; page < 8; page++)
  { // 遍历0~7共8页
    OLED_SendCmd(0xB0 + page);
    OLED_SendCmd(0x00);
    OLED_SendCmd(0x10);
    for (col = 0; col < 128; col++)
    {                      // 遍历当前页的128列
      OLED_SendData(0xFF); // 写入0xFF，当前列8行全亮
    }
  }
}

// 2. 全屏清空（所有像素灭，清屏用，比逐个点清屏高效）
void OLED_Fill_AllOff(void)
{
  uint8_t page, col;

  for (page = 0; page < 8; page++)
  { // 遍历 0~7 共8页
    // 1. 设置当前页地址
    OLED_SendCmd(0xB0 + page);
    // 2. 设置当前列起始地址（0列开始）
    OLED_SendCmd(0x00); // 列地址低4位
    OLED_SendCmd(0x10); // 列地址高4位

    // 3. 当前页128列全部写入 0x00（0x00=8行全灭）
    for (col = 0; col < 128; col++)
    {
      OLED_SendData(0x00);
    }
  }
}

// 【新增核心函数1：设置OLED显示区域（适配16×16字符）】
// start_col: 起始列(0~127)  end_col: 结束列(0~127)
// start_page: 起始页(0~7)  end_page: 结束页(0~7)
void OLED_SetArea(uint8_t start_col, uint8_t end_col, uint8_t start_page, uint8_t end_page)
{
    // 设置列地址范围 (start_col 到 end_col)
    OLED_SendCmd(0x21);
    OLED_SendCmd(start_col);   // 起始列
    OLED_SendCmd(end_col);     // 结束列

    // 设置页地址范围 (start_page 到 end_page)
    OLED_SendCmd(0x22);
    OLED_SendCmd(start_page);  // 起始页
    OLED_SendCmd(end_page);    // 结束页
}


void OLED_ShowNum_12x6(uint8_t x, uint8_t y, uint8_t num)
{
  // 1. 越界检查：12×12区域不超出屏幕
  if (x + 5 > 127 || y + 11 > 63)
    return;

  // 2. 计算数字占用的页/列范围
  uint8_t start_col = x;
  uint8_t end_col = x + 5;        // 12列（x~x+11）
  uint8_t start_page = y / 8;      // 起始页（y行对应页）
  uint8_t end_page = (y + 11) / 8; // 结束页（12行跨2页）

  // 3. 设置显示区域
  OLED_SetArea(start_col, end_col, start_page, end_page);


  OLED_SendData_Batch((uint8_t *)asc2_1206[num], 12);
}



void OLED_ShowHz_12x12(uint8_t x, uint8_t y, uint8_t num)
{
  // 1. 越界检查：12×12区域不超出屏幕
  if (x + 11 > 127 || y + 11 > 63)
    return;

  // 2. 计算数字占用的页/列范围
  uint8_t start_col = x;
  uint8_t end_col = x + 11;        // 12列（x~x+11）
  uint8_t start_page = y / 8;      // 起始页（y行对应页）
  uint8_t end_page = (y + 11) / 8; // 结束页（12行跨2页）


  // 3. 设置显示区域
  OLED_SetArea(start_col, end_col, start_page, end_page);


  OLED_SendData_Batch((uint8_t *)hz_24[num], 24);
}


void OLED_ClearArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    // 1. 越界检查：确保区域不超出OLED屏幕(128x64)
    if (x >= 128 || y >= 64 || width == 0 || height == 0)
        return;
    if (x + width > 128) width = 128 - x;  // 修正超出右边界的宽度
    if (y + height > 64) height = 64 - y;  // 修正超出下边界的高度

    // 2. 计算区域对应的列/页范围（与原显示函数逻辑一致）
    uint8_t start_col = x;
    uint8_t end_col = x + width - 1;       // 列范围：[start_col, end_col]
    uint8_t start_page = y / 8;            // 起始页（8行1页）
    uint8_t end_page = (y + height - 1) / 8; // 结束页

    // 3. 设置待清空的显示区域
    OLED_SetArea(start_col, end_col, start_page, end_page);

    // 4. 计算需要填充的字节数 = 列数 × 页数
    uint16_t fill_len = width * (end_page - start_page + 1);
    
    // 5. 循环发送单个0x00字节，清空区域（无数组版）
    uint8_t clear_data = 0x00;
    for (uint16_t i = 0; i < fill_len; i++)
    {
        HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, 
                          &clear_data, 1, 100);
    }
}







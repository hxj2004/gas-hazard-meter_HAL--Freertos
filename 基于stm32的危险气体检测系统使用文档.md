# 基于stm32的危险气体检测系统使用文档



### 硬件选型

面包板，面包板供电模块,stm32F103c8t6,esp32-C3,mq-2烟雾传感器（可燃气体）,tvoc301（甲醛，污染气体，二氧化碳）,OLED0.96寸,有源蜂鸣器

### 硬件连接

硬件连接原理图

mq2(AO)-stm32-ADC_IN0

DHT11-stm32-PA1

usb2ttl - stm32-UART2

buzzer-stm32-PA4

TVOC301-stm32-UART3

esp32UART1-stm32-UART1

### onenet云平台

将设备名称，设备id，apikey替换

以下为onenet物理模型

```json
{
  "version": "1.0",
  "profile": {
    "industryId": "2",
    "sceneId": "20",
    "categoryId": "281",
    "productId": "SVQx377cH4"
  },
  "properties": [
    {
      "identifier": "ch2o",
      "name": "甲醛",
      "functionType": "u",
      "accessMode": "rw",
      "desc": "",
      "dataType": {
        "type": "float",
        "specs": {
          "max": "10.000",
          "min": "0",
          "step": "",
          "unit": "百万分率 / ppm"
        }
      },
      "functionMode": "property",
      "required": false
    },
    {
      "identifier": "co2",
      "name": "二氧化碳",
      "functionType": "u",
      "accessMode": "rw",
      "desc": "",
      "dataType": {
        "type": "int32",
        "specs": {
          "max": "999999",
          "min": "0",
          "step": "",
          "unit": "百万分率 / ppm"
        }
      },
      "functionMode": "property",
      "required": false
    },
    {
      "identifier": "humid",
      "name": "湿度",
      "functionType": "u",
      "accessMode": "rw",
      "desc": "",
      "dataType": {
        "type": "int32",
        "specs": {
          "max": "100",
          "min": "0",
          "step": "",
          "unit": "百分比 / %"
        }
      },
      "functionMode": "property",
      "required": false
    },
    {
      "identifier": "mq2_value",
      "name": "可燃气体",
      "functionType": "u",
      "accessMode": "rw",
      "desc": "",
      "dataType": {
        "type": "float",
        "specs": {
          "max": "999999",
          "min": "0",
          "step": "",
          "unit": "百万分率 / ppm"
        }
      },
      "functionMode": "property",
      "required": false
    },
    {
      "identifier": "temp",
      "name": "温度",
      "functionType": "u",
      "accessMode": "rw",
      "desc": "",
      "dataType": {
        "type": "int32",
        "specs": {
          "max": "100",
          "min": "-100",
          "step": "",
          "unit": "摄氏度 / °C"
        }
      },
      "functionMode": "property",
      "required": false
    },
    {
      "identifier": "tvoc",
      "name": "污染气体",
      "functionType": "u",
      "accessMode": "rw",
      "desc": "",
      "dataType": {
        "type": "float",
        "specs": {
          "max": "10.000",
          "min": "0",
          "step": "",
          "unit": "百万分率 / ppm"
        }
      },
      "functionMode": "property",
      "required": false
    }
  ],
  "events": [],
  "services": [],
  "combs": []
}
```


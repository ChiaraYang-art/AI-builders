# Wiring

## OLED 单独测试

```text
AtomS3R Grove / HY2.0-4P
-> M5Stack Unit OLED 1.3 inch
```

代码引脚：

```cpp
#define SDA_PIN 2
#define SCL_PIN 1
```

OLED 构造器：

```cpp
U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R1,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);
```

## DLight 单独测试

```text
AtomS3R Grove / HY2.0-4P
-> M5Stack DLight Unit
```

代码引脚和地址：

```cpp
#define SDA_PIN 2
#define SCL_PIN 1
#define BH1750_ADDR 0x23
```

如果读不到 DLight，可以尝试把地址改成 `0x5C`。

## Hub 到货后的目标

```text
AtomS3R Grove / HY2.0-4P
-> Unit Hub / PaHUB
   -> OLED Unit
   -> DLight Unit
   -> ENV Pro
```

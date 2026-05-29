#include <M5Unified.h>

// 定义一些颜色
#define PLANT_GREEN 0xAFE5 // 浅绿色
#define DARK_GREEN  0x2589 // 深绿色 (眼珠)
#define FLOWER_WHITE 0xFFFF // 白色

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(0); 
    M5.Display.fillScreen(TFT_BLACK);
}

void loop() {
    // 演示不同状态的动画
    drawPlant(0); // 正视
    delay(1000);
    drawPlant(1); // 向右看
    delay(1000);
    drawPlant(2); // 向上看
    delay(1000);
    drawPlant(4); // 闭眼
    delay(1000);
}

// 绘制植物的方法，state控制眼睛状态: 0=正视, 1=右视, 2=上视, 3=下视, 4=闭眼
void drawPlant(int state) {
    M5.Display.fillScreen(TFT_BLACK); // 清屏

    // 1. 绘制植物的主体 (用层叠的圆角矩形模拟叶片层)
    M5.Display.fillRoundRect(24, 60, 80, 16, 4, PLANT_GREEN);
    M5.Display.fillRoundRect(16, 72, 96, 16, 4, PLANT_GREEN);
    M5.Display.fillRoundRect(20, 84, 88, 16, 4, PLANT_GREEN);
    M5.Display.fillRoundRect(28, 96, 72, 12, 4, PLANT_GREEN);

    // 2. 绘制头顶的花朵 (用圆形组合模拟)
    // 左花
    M5.Display.fillCircle(40, 50, 14, FLOWER_WHITE);
    // 右花
    M5.Display.fillCircle(88, 50, 14, FLOWER_WHITE);
    // 中花 (稍微靠前一点)
    M5.Display.fillCircle(64, 45, 16, FLOWER_WHITE);

    // 3. 绘制眼睛
    if (state == 4) {
        // 闭眼状态：画两条深绿色的线
        M5.Display.fillRect(44, 80, 12, 4, DARK_GREEN);
        M5.Display.fillRect(72, 80, 12, 4, DARK_GREEN);
    } else {
        // 睁眼状态：画眼白
        M5.Display.fillCircle(50, 80, 8, FLOWER_WHITE);
        M5.Display.fillCircle(78, 80, 8, FLOWER_WHITE);

        // 计算眼珠的偏移量
        int offsetX = 0;
        int offsetY = 0;
        if (state == 1) offsetX = 3;  // 向右看
        if (state == 2) offsetY = -3; // 向上看
        if (state == 3) offsetY = 3;  // 向下看

        // 画眼珠
        M5.Display.fillCircle(50 + offsetX, 80 + offsetY, 4, DARK_GREEN);
        M5.Display.fillCircle(78 + offsetX, 80 + offsetY, 4, DARK_GREEN);
    }
}
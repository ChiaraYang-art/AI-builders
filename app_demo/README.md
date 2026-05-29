# City Sprout App Demo Versions

这个目录按生成时间整理了小芽 App Demo 的不同版本。每个编号文件夹都是一个独立版本，编号越大越新。

## 版本索引

### 01_initial_component_demo

最早的静态组件版。

入口：

```text
01_initial_component_demo/index.html
```

特点：

- HTML/CSS/JS 组件实现
- 不是严格按照最终 Figma prototype
- 适合查看早期产品逻辑草稿

### 02_section2_demo

早期根据 Section 2 尝试生成的版本。

入口：

```text
02_section2_demo/section2_demo.html
```

特点：

- 过渡版本
- 只作为历史参考

### 03_figma_image_hotspot_demo

Figma 图片 + 透明点击热区版本。

入口：

```text
03_figma_image_hotspot_demo/figma_dev_demo.html
```

特点：

- 视觉最接近当时的 Figma 截图
- 页面不是组件化代码
- 适合展示，不适合后续大量改动

### 04_component_v3_draft

第一次组件化 V3 草稿。

入口：

```text
04_component_v3_draft/component_v3.html
```

特点：

- HTML/CSS/JS 组件实现
- 有一些 Codex 补出来的页面和流程
- 不完全严格遵循 Figma prototype

### 05_strict_image_hotspot_demo

严格 Figma 图片热区版。

入口：

```text
05_strict_image_hotspot_demo/component_v3_strict.html
```

特点：

- 使用 Figma frame 本地图片
- 透明热区基本按 prototype 线实现
- 保留两个特殊逻辑：首页说话框随机进入四种邀请页，底部导航“散步”随机进入四种邀请页
- 视觉还原较高，但不是组件化页面

### 06_latest_strict_component_demo

当前最新版本，推荐给程序同学继续开发。

入口：

```text
06_latest_strict_component_demo/component_v3_strict_components.html
```

特点：

- HTML/CSS/JS 组件化实现
- 按最新 Figma prototype 交互线整理
- 首页说话框随机进入四种邀请页
- 底部导航“散步”随机进入四种邀请页
- 支持 colorwalk 小作文长页面滚动
- 分享页包含轻量保存提示和分享底部弹窗

## 当前推荐

如果要继续开发、加动效、接后端数据、接硬件状态，请从这个版本开始：

```text
06_latest_strict_component_demo/component_v3_strict_components.html
```

如果要做高还原视觉展示，可以参考：

```text
05_strict_image_hotspot_demo/component_v3_strict.html
```

## 后续开发建议

- 对卡片、底部导航、按钮、状态文字、小芽插画继续组件化。
- 对图片复杂页面，可以先保留局部图片资产，再逐步替换成组件。
- 对长页面，例如 colorwalk 小作文，需要保留滚动逻辑。
- 对分享、保存图片、APP 分享面板，可以先用当前轻量弹窗，后续再接真实移动端能力。

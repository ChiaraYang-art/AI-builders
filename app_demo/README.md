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

最新的纯 HTML/CSS/JS 组件版。

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
- 可以直接用浏览器打开，不需要安装依赖

### 07_vue_app_demo

第一版 Vue/Vite 版本，适合理解 Vue 工程结构，但视觉不是 Figma 优先版。

入口：

```text
07_vue_app_demo/
```

特点：
- 使用 Vue 3 + Vite
- 从 `06_latest_strict_component_demo` 的交互逻辑迁移
- 已拆出 `BottomNav`、`PhoneHeader`、`SensorList`、`Sprout` 等基础组件
- 更适合后续接后端、加动效、管理页面状态
- 需要 `npm install` 和 `npm run dev` 后才能在浏览器查看

### 08_vue_figma_strict_demo

当前最新版本，推荐给程序同学继续开发。

入口：

```text
08_vue_figma_strict_demo/
```

特点：
- 使用 Vue 3 + Vite
- 视觉优先贴近 Figma 设计稿
- 调用从 Figma 导出的 SVG/PNG 资产
- 小芽、地图、图鉴、日记图片、底部导航图标都来自 `public/assets/figma/`
- 保留首页说话框随机进入四种邀请页、底部“散步”随机进入四种邀请页
- 更适合继续接后端、接硬件状态、加动效

运行：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\08_vue_figma_strict_demo"
npm install
npm run dev
```

运行：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\07_vue_app_demo"
npm install
npm run dev
```

## 当前推荐

如果只是要马上展示、双击打开：

```text
06_latest_strict_component_demo/component_v3_strict_components.html
```

如果要给程序同学继续开发、加动效、接后端：

```text
08_vue_figma_strict_demo/
```

## 后续开发建议

- Vue 版可以继续拆页面组件，例如 `HomeScreen.vue`、`InviteScreen.vue`、`WalkScreen.vue`、`DiaryScreen.vue`。
- 后续如果页面变多，可以引入 Vue Router。
- 传感器状态、后端生成文案、散步记录可以逐步从假数据换成接口数据。
- 图片复杂的页面可以保留局部图片资产，再逐步替换成真正的组件。

# 08 Vue Figma Strict Demo

这是“出走小芽”App Demo 的 Figma 视觉优先 Vue 版本。

和 `07_vue_app_demo` 的区别：

- `07` 更像功能原型，视觉有明显重新设计痕迹。
- `08` 优先贴近 Figma：米色背景、软卡片、绿色导航、导出的 SVG/PNG 小芽和地图资产。
- 复杂插画、地图、图鉴小芽、日记图片优先从 `public/assets/figma/` 调用。
- 文字、按钮、导航、卡片仍然用 Vue + CSS 实现，方便后续接数据和改动效。

## 运行方式

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\08_vue_figma_strict_demo"
npm install
npm run dev
```

浏览器打开终端显示的网址，通常是：

```text
http://localhost:5173/
```

## 当前交互逻辑

- 首页点击小芽说话框，随机进入四种邀请页。
- 底部导航的“散步”随机进入四种邀请页。
- 四种邀请页进入对应散步页。
- 散步页左上返回进入对应暂停页。
- 暂停页可继续散步或完成散步。
- Color Walk 完成后进入 Color Walk 版本日记。
- 普通散步完成后进入普通日记。
- 日记页可进入流水账、小作文、散步地图、分享卡片。
- 我的页可进入小芽图鉴。

## 资产目录

```text
public/assets/figma/
```

这里放的是从 Figma 导出的资产。当前包括：

- `sprout-wilted.svg`
- `sprout-happy.svg`
- `walk-map.svg`
- `walk-map-card.svg`
- `diary-page-sprout.svg`
- `diary-color-walk-photos.svg`
- `share-card-sprout.svg`
- `atlas-*.svg`
- 底部导航图标 SVG
- Color Walk 照片 PNG

## 后续给程序同学的建议

- 如果要继续提高还原度，优先对照 Figma 调整 `src/styles.css` 中的绝对坐标、字号和卡片尺寸。
- 如果某块视觉非常复杂，不需要动态改内容，继续从 Figma 导出 SVG/PNG 后放进 `public/assets/figma/`。
- 如果某块需要接后端数据，例如传感器状态、日记内容、散步统计，就保留为 Vue + CSS 结构。
- 后续可以继续把 `App.vue` 拆成页面组件，例如 `HomeScreen.vue`、`InviteScreen.vue`、`DiaryScreen.vue`、`MapScreen.vue`。

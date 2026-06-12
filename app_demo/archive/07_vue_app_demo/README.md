# 07 Vue App Demo

这是“出走小芽”App Demo 的 Vue/Vite 版本，基于 `06_latest_strict_component_demo` 的交互逻辑迁移而来。

## 适合谁继续开发

如果程序同学后续要继续加动效、接后端、接硬件状态、拆更多组件，建议从这个版本开始。

如果只是要直接双击打开展示，请继续使用 `06_latest_strict_component_demo`，因为 Vue 版本需要先安装依赖并启动开发服务器。

## 运行方式

在 PowerShell 中进入这个文件夹：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\07_vue_app_demo"
```

第一次运行前安装依赖：

```powershell
npm install
```

启动本地预览：

```powershell
npm run dev
```

终端会显示一个本地网址，通常类似：

```text
http://localhost:5173/
```

在浏览器打开这个网址即可查看。

## 当前已实现的产品逻辑

- 首页点击小芽说话框，随机进入四种邀请页：Light Walk、Sound Walk、Color Walk、Local Discovery。
- 底部导航的“散步”也会随机进入四种邀请页。
- 四种邀请页可以进入对应散步页。
- 散步页返回/暂停进入对应暂停层。
- 暂停层可以继续散步或完成散步。
- Color Walk 完成后进入 colorwalk 版本日记。
- 普通散步完成后进入普通日记。
- 日记页支持日历、流水账、小作文、地图、分享卡片。
- “我的”页支持进入小芽图鉴。

## 文件结构

```text
07_vue_app_demo/
├── index.html
├── package.json
├── README.md
└── src/
    ├── App.vue
    ├── main.js
    ├── styles.css
    └── components/
        ├── BottomNav.vue
        ├── PhoneHeader.vue
        ├── SensorList.vue
        └── Sprout.vue
```

## 后续建议

- 把邀请页、散步页、日记页继续拆成独立 Vue 组件。
- 用 Vue Router 替换当前的 `route` 字符串状态。
- 如果要接 Flask 或云端后端，可以先在 `App.vue` 中把传感器假数据改成接口请求。
- 如果要加动效，优先给 `Sprout.vue`、页面切换、暂停层和完成反馈加动画。

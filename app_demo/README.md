# City Sprout App Demo

这个目录存放小芽前端 Demo。默认请从当前版本开始阅读和运行；历史版本已经移动到 `archive/`，用于追溯设计和实现演变。

## 当前推荐版本

| 版本 | 路径 | 用途 |
| --- | --- | --- |
| 手机扫码 Demo | `09_vue_mobile_qr_demo/` | 当前最适合路演、手机展示和二维码访问的版本 |
| Figma 严格版 Demo | `08_vue_figma_strict_demo/` | 当前桌面开发主线，视觉优先贴近 Figma，适合继续联调后端和硬件 |
| Figma 原始资产 | `figma_assets_raw/` | 从 Figma 导出的 SVG/PNG 原始素材 |

## 运行手机扫码 Demo

```powershell
cd app_demo/09_vue_mobile_qr_demo
npm install
npm run dev:mobile
```

如果只在本机浏览，也可以运行：

```powershell
npm run dev
```

## 运行桌面/Figma 严格版 Demo

```powershell
cd app_demo/08_vue_figma_strict_demo
npm install
npm run dev
```

两个 Vue 项目都会通过 Vite 代理连接 Flask 后端。后端默认运行在 `http://127.0.0.1:5000`，配置示例见各自目录下的 `.env.example`。

## 历史版本

`archive/` 中保留了从静态 HTML、图片热区、组件化草稿到第一版 Vue 的历史过程：

```text
archive/
  01_initial_component_demo/
  02_section2_demo/
  03_figma_image_hotspot_demo/
  04_component_v3_draft/
  05_strict_image_hotspot_demo/
  06_latest_strict_component_demo/
  07_vue_app_demo/
```

这些版本不再作为当前开发入口，但对理解“从 Figma 截图热区到 Vue 组件化”的演进很有价值。

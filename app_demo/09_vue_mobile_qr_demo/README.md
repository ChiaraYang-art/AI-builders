# 出走小芽手机扫码访问版

这是基于 `08_vue_figma_strict_demo` 拆出的手机扫码体验版。它保留原来的 Vue Router、页面组件、Figma 导出资源和后端 API 接入，但入口改成手机优先：扫码打开后直接显示完整手机应用，不再显示桌面演示说明面板。

## 与 08 版本的区别

- 默认全屏手机体验，适合手机浏览器、微信内置浏览器扫码访问。
- 自动按真实视口缩放 393 x 852 的 Figma 手机稿，兼容 375 宽 iPhone 和常见安卓机。
- `index.html` 增加 `viewport-fit=cover`、主题色和 iOS Web App 元信息。
- Vite 支持 `VITE_PUBLIC_BASE`，方便部署到域名根路径或子路径。
- README 和 `.env.example` 改为扫码部署说明。

## 本地运行

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\09_vue_mobile_qr_demo"
npm install
npm run dev
```

固定端口预览：

```powershell
npm run dev:mobile
```

如果需要接 Flask 后端，复制 `.env.example` 为 `.env.local`，按实际后端地址修改：

```text
VITE_API_BASE=/api
VITE_API_PROXY_TARGET=http://127.0.0.1:5000
VITE_PUBLIC_BASE=/
```

## 打包部署

### Vercel 快速部署

Vercel 会自动给项目分配 HTTPS 域名，例如：

```text
https://city-sprout-mobile-qr-demo.vercel.app/#/home
```

从本目录执行：

```powershell
npm install
npm run build
npx vercel login
npx vercel --prod
```

第一次部署时按提示选择：

- Set up and deploy: `Y`
- Which scope: 选择你的账号
- Link to existing project: `N`
- Project name: 可用 `city-sprout-mobile-qr-demo`
- In which directory is your code located: `./`
- Override settings: `N`

部署完成后，Vercel 输出的 Production URL 就是二维码地址。建议二维码填：

```text
https://你的-vercel-地址/#/home
```

如果部署在域名根路径，例如：

```text
https://your-domain.com/
```

保持：

```text
VITE_PUBLIC_BASE=/
```

然后执行：

```powershell
npm run build
```

把生成的 `dist/` 目录内容上传到站点根目录。

本地检查生产包：

```powershell
npm run preview:mobile
```

如果部署在子路径，例如：

```text
https://your-domain.com/citysprout/
```

打包前设置：

```text
VITE_PUBLIC_BASE=/citysprout/
```

然后执行 `npm run build`，把 `dist/` 内容上传到对应的 `citysprout/` 目录。

## 二维码地址

路由使用 hash 模式，服务器不需要额外配置前端路由回退。二维码建议指向：

```text
https://your-domain.com/#/home
```

或子路径：

```text
https://your-domain.com/citysprout/#/home
```

如果要直接进入某个散步邀请页，也可以用：

```text
https://your-domain.com/#/invite/light
https://your-domain.com/#/invite/sound
https://your-domain.com/#/invite/color
https://your-domain.com/#/invite/local
```

## 上线前检查

- 域名必须启用 HTTPS，手机端相机、麦克风、定位、音频播放等能力会依赖安全上下文。
- 后端接口如果不是同域，需要配置 CORS 或通过同域反向代理转发 `/api`。
- 用 iPhone Safari、安卓 Chrome、微信内置浏览器各扫一次二维码。
- 重点检查底部导航、录音/上传照片、分享页、地图页是否被浏览器地址栏或安全区遮挡。

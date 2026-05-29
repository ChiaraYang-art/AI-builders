# City Sprout App Demo

这是一个不依赖后端的静态软件界面 Demo，用来演示 Figma Unified V3 的核心交互流程。

你需要下载figma app
并在codex开启figma插件

## 使用方式

直接用浏览器打开：

```text
app_demo/index.html
看这一版就可以，其他是旧版
```

## 当前交互

- 第一屏“有点蔫了”中，点击小芽说话框，会随机进入以下任意一个出门邀请：
  - Light Walk
  - Sound Walk
  - Color Walk
  - Local Discovery
- 出门邀请页可以开始散步，也可以换成更轻的任务。
- 散步进行中可以暂停、中途退出或结束散步。
- 结束散步后可以生成小芽反馈、写入日记、分享卡片。

## 设计说明

这版 Demo 使用 Unified V3 的视觉语言：

- 奶油白背景
- 柔和绿色主色
- 圆角卡片
- 小芽插画
- 底部五栏导航
- 字体优先使用 `Alibaba PuHuiTi 3.0`

## 遇到的问题-codex对话记录
### 它说遇到了“本地下载 Figma PNG 被 Windows 的 TLS 凭证问题挡住了。的问题

已按你的确认做了新版，不覆盖之前文件。新入口是：

[figma_dev_demo.html](</C:/Users/yqe/Desktop/CitySproutDemo/AI builders/app_demo/figma_dev_demo.html>)

这版采用“Figma 帧截图 + 透明点击热区”的方式，所以视觉会比我之前用 CSS 重画的版本更接近设计稿。已实现：

- 首页点击“小芽说话框”随机进入四种邀请：
  - lightwalk
  - soundwalk
  - colrwalk
  - local discovery
- 按 Figma 交互线进入对应任务页
- light walk / sound walk 的暂停弹窗、继续、完成
- 日记、流水账、小作文、分享卡片、地图、附近、我的、图鉴等主要跳转
- 没有新增你没画的页面

新增文件：

- [figma_dev_demo.html](</C:/Users/yqe/Desktop/CitySproutDemo/AI builders/app_demo/figma_dev_demo.html>)
- [figma_dev_demo.css](</C:/Users/yqe/Desktop/CitySproutDemo/AI builders/app_demo/figma_dev_demo.css>)
- [figma_dev_demo.js](</C:/Users/yqe/Desktop/CitySproutDemo/AI builders/app_demo/figma_dev_demo.js>)

验证：`figma_dev_demo.js` 已通过 `node --check`。

注意：当前图片用的是 Figma MCP 导出的临时图片链接。视觉还原最准确，但链接可能会过期。后续如果要长期展示版，需要把这些 PNG 从 Figma 导出成本地文件，再把 JS 里的 URL 换成本地路径。

### 我问他怎么解决 它说：
这是 Windows 自带 `curl.exe` 的 TLS/Schannel 问题，不是 Figma 文件问题。报错：

`schannel: AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS`

意思是 Windows 的 TLS 层在建立 HTTPS 连接时拿不到它想要的凭证/证书上下文。常见于临时 HTTPS 资源、公司/校园网络代理、证书吊销检查、Windows curl 和某些服务器握手不兼容。

可以这样解决，按推荐顺序试：

**方案 1：用浏览器直接下载**
把 Figma MCP 返回的图片 URL 粘到浏览器里打开，然后右键保存 PNG。  
这是最稳的，因为浏览器 TLS/证书处理比 PowerShell/curl 稳。

**方案 2：用 Python 下载，绕开 Windows curl**
在 PowerShell 里可以用：

```powershell
python -c "import urllib.request; urllib.request.urlretrieve('FIGMA_IMAGE_URL', 'output.png')"
```

把 `FIGMA_IMAGE_URL` 换成 Figma 返回的图片地址。

如果 `urllib` 也失败，可以装 `requests` 后用：

```powershell
python -m pip install requests certifi
python -c "import requests; u='FIGMA_IMAGE_URL'; r=requests.get(u, timeout=30); r.raise_for_status(); open('output.png','wb').write(r.content)"
```

**方案 3：用 Git Bash / WSL 的 curl**
Windows 自带 curl 用 Schannel；Git Bash/WSL 的 curl 通常用 OpenSSL，问题少很多：

```bash
curl -L -o output.png "FIGMA_IMAGE_URL"
```

**方案 4：试 curl 禁用证书吊销检查**
有时是 Windows 证书吊销检查卡住：

```powershell
curl.exe --ssl-no-revoke -L -o output.png "FIGMA_IMAGE_URL"
```

如果还报 `SEC_E_NO_CREDENTIALS`，就别继续折腾 Windows curl，直接用 Python 或浏览器。

另外注意：Figma MCP 图片链接是短期链接，过一段时间会失效。最好拿到链接后尽快下载。
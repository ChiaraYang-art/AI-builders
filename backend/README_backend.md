# City Sprout Backend

运行后端：

```powershell
python backend/sprout_server.py
```

AtomS3R 访问时不要使用 `127.0.0.1`。请使用 Flask 启动日志里显示的局域网地址，例如：

```text
http://192.168.10.36:5000/plant
```

浏览器语音模拟页面：

```text
http://127.0.0.1:5000/
```

先点击 `Enable Voice`，浏览器才允许自动朗读后续植物文案。

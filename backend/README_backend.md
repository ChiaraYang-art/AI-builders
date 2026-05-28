# City Sprout Backend

Run the Flask backend:

```powershell
python backend/sprout_server.py
```

For local hardware testing, do not use `127.0.0.1` in Arduino code. Use the LAN address printed by Flask, for example:

```text
http://your-lan-ip:5000/plant
```

For cloud deployment, set secrets as environment variables instead of committing them:

```bash
export DASHSCOPE_API_KEY="your-api-key"
```

The browser voice page is available at:

```text
http://your-server-host:5000/
```

Click `Enable Audio` or `Enable Voice` in the browser before playback. Browsers block autoplay until the user interacts with the page.

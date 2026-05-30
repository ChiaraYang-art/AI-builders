"""兼容入口：保留 `python sprout_server.py` 与 gunicorn sprout_server:app。"""

from app import app

if __name__ == "__main__":
    from config import DEFAULT_PORT

    app.run(host="0.0.0.0", port=DEFAULT_PORT)

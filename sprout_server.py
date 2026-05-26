"""
出走小芽 Demo - 电脑端小后端

这个程序运行在你的电脑上。

它的作用：
1. 接收 AtomS3R 发来的光照数据 lux
2. 根据 lux 生成一句“植物语言”
3. 把这句话返回给 AtomS3R
4. AtomS3R 再把这句话显示在 OLED 上

注意：
这一版先不调用真正 AI，而是用规则模拟 AI。
这样做是为了先把“硬件 → 后端 → 返回文案”的流程跑通。
后面你们可以把 generate_plant_sentence() 函数替换成真正的大模型 API 调用。
"""

from flask import Flask, request

# 创建一个 Flask 应用
# 你可以把它理解成：电脑上开了一个“小型网站服务器”
app = Flask(__name__)


def generate_plant_sentence(lux: float) -> str:
    """
    根据光照 lux 生成一句植物语言。

    参数：
    lux: 光照传感器读到的数值，单位是 lux。

    返回：
    一句适合显示在 OLED 上的英文短句。

    为什么先用英文？
    因为 OLED 显示中文需要额外字库，会比英文复杂。
    第一版 demo 先用短英文最稳。
    """

    # 光照很低：植物觉得缺光
    if lux < 50:
        return "I feel wilted. Need sun."

    # 中等光照：植物感觉到一点光
    elif lux < 500:
        return "I feel a little light."

    # 强光照：植物晒到太阳
    else:
        return "Sunlight found. Alive!"


@app.route("/plant", methods=["POST"])
def plant():
    """
    这个接口专门给 AtomS3R 调用。

    AtomS3R 会用 HTTP POST 请求，把 lux 发过来。
    比如发来：
    {"lux": 123.4}

    电脑收到后，生成一句植物语言并返回。
    """

    # 读取 AtomS3R 发来的 JSON 数据
    data = request.get_json(force=True)

    # 从数据里取出 lux
    # 如果没有 lux，就默认是 0
    lux = float(data.get("lux", 0))

    # 在电脑终端里打印出来，方便你调试
    print(f"Received lux: {lux}")

    # 生成植物语言
    sentence = generate_plant_sentence(lux)

    # 再打印一下返回内容
    print(f"Return sentence: {sentence}")

    # 直接返回纯文本
    # 这样 AtomS3R 端解析起来最简单
    return sentence


if __name__ == "__main__":
    """
    启动服务器。

    host="0.0.0.0" 的意思是：
    允许同一个 Wi-Fi 下的其他设备访问你的电脑。

    port=5000 的意思是：
    服务器运行在 5000 端口。
    """
    app.run(host="0.0.0.0", port=5000)
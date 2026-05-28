FROM python:3.12-slim

# 国内服务器构建时优先使用镜像源（可在 build-arg 覆盖）
ARG PIP_INDEX_URL=https://pypi.tuna.tsinghua.edu.cn/simple
ENV PIP_INDEX_URL=${PIP_INDEX_URL} \
    PIP_TRUSTED_HOST=pypi.tuna.tsinghua.edu.cn \
    PIP_DEFAULT_TIMEOUT=300

# amd64 + Python 3.12 下 cryptography 等有预编译 wheel，无需 apt 装 gcc（避免国内 apt 极慢导致 CD 超时）
WORKDIR /app

COPY deploy/requirements-prod.txt /app/requirements-prod.txt
RUN python -m pip install --no-cache-dir --upgrade pip setuptools wheel \
    && python -m pip install --no-cache-dir -r /app/requirements-prod.txt

COPY backend/ /app/backend/

WORKDIR /app/backend

RUN mkdir -p /app/backend/generated

EXPOSE 5000

CMD ["gunicorn", "--bind", "0.0.0.0:5000", "--workers", "2", "--timeout", "120", "sprout_server:app"]

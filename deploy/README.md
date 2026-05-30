# 生产部署（Docker Compose + CD）

`main` 分支更新后，通过 GitHub Actions SSH 触发服务器执行 `deploy/deploy.sh`：在 `/opt/AI-builders` 拉取代码并用 Docker Compose 重新构建、启动 Flask 后端。

## 架构

```text
开发者 push main
    -> GitHub Actions (cd.yml)
    -> SSH 到服务器
    -> deploy/deploy.sh
    -> git pull + docker compose up -d --build
    -> sprout-server 容器 :5000
```

## 服务器目录

| 路径 | 说明 |
|------|------|
| `/opt/AI-builders` | 代码仓库（可通过 `DEPLOY_REPO_DIR` 覆盖） |
| `/opt/AI-builders/deploy/.env` | 运行时配置（不提交 git） |

## 首次初始化（仅需一次）

在目标 Linux 服务器上：

1. 配置能 `git pull` 的 SSH 部署密钥（对 GitHub 仓库只读即可）。
2. 运行引导脚本：

```bash
export DEPLOY_REPO_URL=git@github.com:ChiaraYang-art/AI-builders.git
sudo bash /path/to/bootstrap.sh
```

或手动克隆后部署：

```bash
sudo git clone git@github.com:ChiaraYang-art/AI-builders.git /opt/AI-builders
cd /opt/AI-builders
cp deploy/.env.example deploy/.env
chmod +x deploy/deploy.sh
sudo ./deploy/deploy.sh
```

## GitHub Actions 密钥

在仓库 **Settings → Secrets and variables → Actions** 添加：

| Secret | 说明 |
|--------|------|
| `SSH_HOST` | 服务器 IP 或域名 |
| `SSH_USER` | SSH 用户名（需能执行 docker、读写 `/opt/AI-builders`） |
| `SSH_PRIVATE_KEY` | 对应用户的私钥（PEM 全文） |
| `SSH_PORT` | 可选，默认 22 |

`SSH_USER` 须已加入 `docker` 组，或对 `deploy.sh` 使用 sudo（默认脚本不 sudo）。

## 日常运维

```bash
cd /opt/AI-builders

# 手动部署（与 CD 相同）
./deploy/deploy.sh

# 查看状态
docker compose -f deploy/docker-compose.yml --project-directory deploy ps

# 查看日志
docker compose -f deploy/docker-compose.yml --project-directory deploy logs -f sprout-server

# 停止
docker compose -f deploy/docker-compose.yml --project-directory deploy down
```

## 硬件联调

容器默认映射宿主机 `5000` 端口。Arduino 中 `SERVER_URL` 应指向：

```text
http://<服务器局域网IP>:5000/plant
```

修改端口时编辑 `deploy/.env` 中的 `SPROUT_PORT`，重新执行 `./deploy/deploy.sh`。

## 环境变量

见 `deploy/.env.example`。复制为 `deploy/.env` 后修改。

## CD 超时（Run Command Timeout）

国内服务器首次 `docker build` 若安装 `gcc`（apt），可能耗时 10 分钟以上，超过 Actions SSH 默认 12 分钟限制。

当前方案：

- `Dockerfile` 不再 `apt install gcc`（依赖使用 PyPI 预编译 wheel）
- `.github/workflows/cd.yml` 将 `command_timeout` 提高到 **40m**

若仍超时，可在服务器手动执行 `./deploy/deploy.sh`（无 SSH 时限）。

## Docker 构建失败（pip / dashscope）

`main` 分支若包含 `dashscope`（百炼 TTS），在国内服务器构建时请确保：

1. 使用 `deploy/Dockerfile` 中的清华 PyPI 镜像（已默认配置）。
2. `requirements-prod.txt` 使用固定版本号，避免依赖解析失败。
3. 服务器上手动重建：`cd /opt/AI-builders && ./deploy/deploy.sh`

若仍失败，在服务器执行查看完整日志：

```bash
cd /opt/AI-builders
docker compose -f deploy/docker-compose.yml --project-directory deploy build --no-cache --progress=plain 2>&1 | tail -80
```

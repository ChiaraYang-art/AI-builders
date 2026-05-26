#!/usr/bin/env bash
# 在服务器上执行：拉取 main 最新代码并用 Docker Compose 重新部署
set -euo pipefail

REPO_DIR="${DEPLOY_REPO_DIR:-/opt/AI-builders}"
BRANCH="${DEPLOY_BRANCH:-main}"
COMPOSE_FILE="${REPO_DIR}/deploy/docker-compose.yml"

log() {
  printf '[deploy] %s\n' "$*"
}

die() {
  printf '[deploy] ERROR: %s\n' "$*" >&2
  exit 1
}

if [[ ! -d "${REPO_DIR}/.git" ]]; then
  die "仓库目录不存在或不是 git 仓库: ${REPO_DIR}"
fi

if ! command -v docker >/dev/null 2>&1; then
  die "未找到 docker，请先运行 deploy/bootstrap.sh"
fi

if ! docker compose version >/dev/null 2>&1; then
  die "未找到 docker compose 插件"
fi

cd "${REPO_DIR}"

if [[ ! -f deploy/env.example ]]; then
  die "缺少 deploy/env.example，请确认仓库路径正确"
fi

if [[ ! -f deploy/.env ]]; then
  log "首次部署：从 env.example 创建 deploy/.env"
  cp deploy/env.example deploy/.env
fi

log "拉取 origin/${BRANCH} ..."
git fetch origin "${BRANCH}"
git checkout "${BRANCH}"
git reset --hard "origin/${BRANCH}"

log "构建并启动容器 ..."
docker compose -f "${COMPOSE_FILE}" --project-directory "${REPO_DIR}/deploy" up -d --build --remove-orphans

log "清理未使用的构建缓存（可选）..."
docker image prune -f --filter "label=com.docker.compose.project=ai-builders" 2>/dev/null || true

log "部署完成"
docker compose -f "${COMPOSE_FILE}" --project-directory "${REPO_DIR}/deploy" ps

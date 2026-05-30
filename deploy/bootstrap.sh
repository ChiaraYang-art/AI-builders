#!/usr/bin/env bash
# 在全新 Linux 服务器上首次安装：Docker、克隆仓库、首次部署
# 用法: sudo bash bootstrap.sh
set -euo pipefail

REPO_DIR="${DEPLOY_REPO_DIR:-/opt/AI-builders}"
REPO_URL="${DEPLOY_REPO_URL:-git@github.com:ChiaraYang-art/AI-builders.git}"
BRANCH="${DEPLOY_BRANCH:-main}"

log() {
  printf '[bootstrap] %s\n' "$*"
}

die() {
  printf '[bootstrap] ERROR: %s\n' "$*" >&2
  exit 1
}

if [[ "$(id -u)" -ne 0 ]]; then
  die "请使用 root 运行: sudo bash bootstrap.sh"
fi

install_docker() {
  if command -v docker >/dev/null 2>&1 && docker compose version >/dev/null 2>&1; then
    log "Docker 已安装"
    return
  fi

  log "安装 Docker ..."
  apt-get update -qq
  apt-get install -y -qq ca-certificates curl git
  curl -fsSL https://get.docker.com | sh
  systemctl enable --now docker
}

install_docker

DEPLOY_USER="${SUDO_USER:-${DEPLOY_USER:-root}}"
if [[ "${DEPLOY_USER}" != "root" ]]; then
  usermod -aG docker "${DEPLOY_USER}" || true
  log "已将用户 ${DEPLOY_USER} 加入 docker 组（重新登录后生效）"
fi

mkdir -p "$(dirname "${REPO_DIR}")"

if [[ ! -d "${REPO_DIR}/.git" ]]; then
  log "克隆仓库到 ${REPO_DIR} ..."
  git clone --branch "${BRANCH}" "${REPO_URL}" "${REPO_DIR}"
else
  log "仓库已存在: ${REPO_DIR}"
fi

if [[ ! -f "${REPO_DIR}/deploy/.env" ]]; then
  cp "${REPO_DIR}/deploy/.env.example" "${REPO_DIR}/deploy/.env"
fi

chmod +x "${REPO_DIR}/deploy/deploy.sh"

log "执行首次部署 ..."
DEPLOY_REPO_DIR="${REPO_DIR}" DEPLOY_BRANCH="${BRANCH}" bash "${REPO_DIR}/deploy/deploy.sh"

log "完成。请将 GitHub Actions 密钥配置好后，push 到 main 即可自动部署。"

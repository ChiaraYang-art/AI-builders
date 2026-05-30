#!/usr/bin/env bash
# City Sprout 一键全链路验证
# 对照 PRD：PRD_CitySprout_软硬件一体版_2026-05-28.md 第 18、19 节
#
# 用法：
#   ./scripts/verify_city_sprout.sh
#   CITY_SPROUT_BASE_URL=http://192.168.43.56:5000 ./scripts/verify_city_sprout.sh
#   ./scripts/verify_city_sprout.sh --skip-tts-wait
#   ./scripts/verify_city_sprout.sh --with-vue-build
#
# 前置：Flask 后端已启动
#   cd backend && python app.py

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BASE_URL="${CITY_SPROUT_BASE_URL:-http://127.0.0.1:5000}"
SKIP_TTS_WAIT=0
WITH_VUE_BUILD=0
WITH_DIALOGUE_TEST=0
IN_PROCESS=0
TTS_TIMEOUT=45

usage() {
  cat <<'EOF'
City Sprout 全链路验证脚本

选项：
  --base-url URL        后端地址（默认 http://127.0.0.1:5000 或 CITY_SPROUT_BASE_URL）
  --skip-tts-wait       不等待 CosyVoice TTS ready（无 Key 或 CI 时用）
  --with-vue-build      额外执行 npm run build 验证 Vue Demo 可编译
  --with-dialogue-test  额外运行 tests/test_dialogue_api.py（Flask test_client）
  --in-process          不依赖已启动的 Flask，直接用 test_client 验证
  --tts-timeout SEC     等待 TTS ready 超时（默认 45 秒）
  -h, --help            显示帮助

示例：
  cd backend && python app.py
  ./scripts/verify_city_sprout.sh
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --base-url)
      BASE_URL="$2"
      shift 2
      ;;
    --skip-tts-wait)
      SKIP_TTS_WAIT=1
      shift
      ;;
    --with-vue-build)
      WITH_VUE_BUILD=1
      shift
      ;;
    --with-dialogue-test)
      WITH_DIALOGUE_TEST=1
      shift
      ;;
    --in-process)
      IN_PROCESS=1
      shift
      ;;
    --tts-timeout)
      TTS_TIMEOUT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "未知参数: $1" >&2
      usage
      exit 2
      ;;
  esac
done

log() {
  printf '\033[1;34m[%s]\033[0m %s\n' "verify" "$1"
}

fail() {
  printf '\033[1;31m[error]\033[0m %s\n' "$1" >&2
  exit 1
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    fail "缺少命令: $1"
  fi
}

require_cmd curl
require_cmd python

log "项目目录: $ROOT_DIR"
log "后端地址: $BASE_URL"

if [[ "$IN_PROCESS" -eq 1 ]]; then
  log "使用 in-process 模式（无需 curl 预检）"
else
  log "检查 Flask 是否可达..."
  if ! curl -fsS --max-time 5 "${BASE_URL}/latest" >/dev/null; then
    cat >&2 <<EOF
无法访问 ${BASE_URL}/latest

请先启动后端：
  cd backend
  python app.py

或使用 in-process 模式（不依赖已启动服务）：
  ./scripts/verify_city_sprout.sh --in-process

若设备在局域网访问，请设置：
  export CITY_SPROUT_BASE_URL=http://<电脑IP>:5000
EOF
    exit 1
  fi
  log "Flask 可达"
fi

PY_ARGS=(--base-url "$BASE_URL" --tts-timeout "$TTS_TIMEOUT")
if [[ "$SKIP_TTS_WAIT" -eq 1 ]]; then
  PY_ARGS+=(--skip-tts-wait)
fi
if [[ "$IN_PROCESS" -eq 1 ]]; then
  PY_ARGS+=(--in-process)
fi

log "运行 API 全链路验证..."
(
  cd "$ROOT_DIR/backend" || exit 1
  python -m tests.verify_full_chain "${PY_ARGS[@]}"
) || fail "API 全链路验证失败"

if [[ "$WITH_DIALOGUE_TEST" -eq 1 ]]; then
  log "运行 test_dialogue_api.py ..."
  (
    cd "$ROOT_DIR/backend" || exit 1
    python -m tests.test_dialogue_api
  ) || fail "test_dialogue_api.py 失败"
fi

if [[ "$WITH_VUE_BUILD" -eq 1 ]]; then
  require_cmd npm
  log "运行 Vue Demo build ..."
  (
    cd "$ROOT_DIR/app_demo/08_vue_figma_strict_demo" || exit 1
    npm run build
  ) || fail "Vue build 失败"
fi

cat <<EOF

============================================================
自动化 API 验证已通过。

建议继续人工验收（见 docs/VERIFICATION_CitySprout.md）：
  1. 硬件 POST /plant + Voice Base 播放 TTS
  2. Vue http://localhost:5173 首页 /latest 同步
  3. Color / Sound 散步页面上传
============================================================
EOF

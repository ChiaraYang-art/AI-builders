# City Sprout 全链路验证（Windows PowerShell）
# 用法：
#   .\scripts\verify_city_sprout.ps1
#   .\scripts\verify_city_sprout.ps1 -InProcess
#   .\scripts\verify_city_sprout.ps1 -SkipTtsWait
#   $env:CITY_SPROUT_BASE_URL="http://192.168.43.56:5000"; .\scripts\verify_city_sprout.ps1

param(
    [string]$BaseUrl = $(if ($env:CITY_SPROUT_BASE_URL) { $env:CITY_SPROUT_BASE_URL } else { "http://127.0.0.1:5000" }),
    [switch]$InProcess,
    [switch]$SkipTtsWait,
    [switch]$WithVueBuild,
    [int]$TtsTimeout = 45
)

$ErrorActionPreference = "Stop"
$RootDir = Split-Path -Parent $PSScriptRoot
$BackendDir = Join-Path $RootDir "backend"

Write-Host "[verify] 项目目录: $RootDir" -ForegroundColor Cyan
Write-Host "[verify] 后端地址: $BaseUrl" -ForegroundColor Cyan

if (-not $InProcess) {
    Write-Host "[verify] 检查 Flask 是否可达..." -ForegroundColor Cyan
    try {
        Invoke-RestMethod -Uri "$BaseUrl/latest" -TimeoutSec 5 | Out-Null
    } catch {
        Write-Host @"

无法访问 $BaseUrl/latest

请先启动后端（务必用最新代码，否则缺少 active_walk 等 P1 字段）：
  cd backend
  python sprout_server.py

或使用进程内模式（不依赖已启动服务）：
  .\scripts\verify_city_sprout.ps1 -InProcess

局域网硬件请设置：
  `$env:CITY_SPROUT_BASE_URL='http://<电脑IP>:5000'
  .\scripts\verify_city_sprout.ps1
"@ -ForegroundColor Red
        exit 1
    }
    Write-Host "[verify] Flask 可达" -ForegroundColor Cyan
}

$pyArgs = @("-m", "tests.verify_full_chain", "--base-url", $BaseUrl, "--tts-timeout", "$TtsTimeout")
if ($SkipTtsWait) { $pyArgs += "--skip-tts-wait" }
if ($InProcess) { $pyArgs += "--in-process" }

Write-Host "[verify] 运行 API 全链路验证..." -ForegroundColor Cyan
Push-Location $BackendDir
try {
    & python @pyArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
    Pop-Location
}

if ($WithVueBuild) {
    Write-Host "[verify] 运行 Vue Demo build ..." -ForegroundColor Cyan
    $vueDir = Join-Path $RootDir "app_demo\08_vue_figma_strict_demo"
    Push-Location $vueDir
    try {
        npm run build
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    } finally {
        Pop-Location
    }
}

Write-Host @"

============================================================
自动化 API 验证已通过。

请继续人工软硬件联调（见 docs/VERIFICATION_CitySprout.md 第 3 节）：
  1. 重启 sprout_server.py 后再跑本脚本（HTTP 模式）
  2. 硬件串口：Send to server / MP3 playback / 中文 Plant says
  3. Vue：npm run dev → http://localhost:5173/#/home
============================================================
"@ -ForegroundColor Green

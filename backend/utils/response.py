"""HTTP 响应相关小工具。"""

from __future__ import annotations


def wants_json_response(accept_header: str | None) -> bool:
    accept = accept_header or ""
    return "application/json" in accept

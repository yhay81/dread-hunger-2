from __future__ import annotations

from collections.abc import Mapping, Sequence
from typing import Any


DEFAULT_REDACT_FIELDS = {
    "token",
    "secret",
    "password",
    "authorization",
    "steam_id",
    "external_chat_id",
    "chat_account_id",
    "ip",
}


def redact_mapping(value: Mapping[str, Any], redact_fields: set[str] | None = None) -> dict[str, Any]:
    fields = {field.lower() for field in (redact_fields or DEFAULT_REDACT_FIELDS)}
    redacted: dict[str, Any] = {}

    for key, item in value.items():
        normalized_key = key.lower()
        if any(field in normalized_key for field in fields):
            redacted[key] = "[REDACTED]"
        elif isinstance(item, Mapping):
            redacted[key] = redact_mapping(item, fields)
        elif isinstance(item, Sequence) and not isinstance(item, (str, bytes, bytearray)):
            redacted[key] = [
                redact_mapping(element, fields) if isinstance(element, Mapping) else element
                for element in item
            ]
        else:
            redacted[key] = item

    return redacted

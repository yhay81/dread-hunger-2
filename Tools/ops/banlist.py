from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass(frozen=True)
class BanEntry:
    platform_user_id: str
    scope: str
    reason: str
    created_at: str


def load_banlist(path: Path) -> list[BanEntry]:
    if not path.exists():
        return []

    data = json.loads(path.read_text(encoding="utf-8"))
    return [BanEntry(**item) for item in data.get("bans", [])]


def save_banlist(path: Path, bans: list[BanEntry]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {"bans": [asdict(entry) for entry in bans]}
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")

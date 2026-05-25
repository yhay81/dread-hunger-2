from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class ServerListing:
    server_id: str
    name: str
    region: str
    map_name: str
    players: int
    max_players: int
    ping_ms: int | None = None


class SteamRegistry:
    """Placeholder adapter for Steam-native server discovery.

    Phase 1 does not call Steam APIs. This class defines the boundary that Phase 2
    will replace with OnlineSubsystem/Steamworks integration.
    """

    def list_servers(self) -> list[ServerListing]:
        return []

from dataclasses import dataclass, field
from secrets import token_hex


@dataclass(frozen=True)
class IncantationCall:
    caller_id: str
    direction: int
    level: int
    created_turn: int
    session_id: str


@dataclass
class CoordinationState:
    player_level: int = 1
    team_name: str = ""
    player_id: str = field(default_factory=lambda: token_hex(8))
    coordination_turn: int = 0
    incantation_call: IncantationCall | None = None
    last_followed_call: tuple | None = None
    ready_player_id: str | None = None
    waiting_for_incantation: bool = False
    team_peers: dict[str, int] = field(default_factory=dict)


STATE = CoordinationState()

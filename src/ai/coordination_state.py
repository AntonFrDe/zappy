from dataclasses import dataclass, field
from secrets import token_hex


@dataclass(frozen=True)
class TeamPlayerStatus:
    level: int
    updated_turn: int
    inventory: dict


@dataclass(frozen=True)
class IncantationCall:
    caller_id: str
    direction: int
    level: int
    created_turn: int
    session_id: str = ""
    cohort: tuple = ()


@dataclass(frozen=True)
class ReadyPlayer:
    updated_turn: int
    session_id: str


@dataclass
class CoordinationState:
    player_level: int = 1
    team_name: str = ""
    team_players: dict = field(default_factory=dict)
    team_cohort: tuple = ()
    last_followed_call: tuple | None = None
    player_id: str = field(default_factory=lambda: token_hex(8))
    incantation_calls: dict = field(default_factory=dict)
    ready_players: dict = field(default_factory=dict)
    team_status_turns: int = 0
    coordination_turn: int = 0
    waiting_for_incantation: bool = False
    arrival_confirmation_caller: tuple | None = None
    incantation_session_id: str | None = None
    incantation_session_started_turn: int | None = None


STATE = CoordinationState()

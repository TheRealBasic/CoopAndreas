# Cheat Sync Matrix

## Synced cheat effects (allowlist)

These effects are host-authoritative and are relayed by the server when received via `CHEAT_EFFECT_TRIGGER`:

- **World time/weather**
  - Relayed through existing `GAME_WEATHER_TIME` handling.
- **Local player wanted level toggles**
  - Relayed through existing `PLAYER_WANTED_LEVEL` handling.
- **Local player synced stats + money**
  - Relayed through existing `PLAYER_STATS` handling.

## Explicitly non-synced / excluded

The following cheat classes remain local-only by design and are **not** relayed from `CHEAT_EFFECT_TRIGGER`:

- Spawning/debug/dev cheats (entity spawning, mission-debug toggles, script debug shortcuts).
- Engine-unsafe or non-deterministic debug cheats.
- Any cheat that is not in the explicit allowlist enum in packet handlers.

Unknown cheat effect types are dropped server-side.

## Anti-spam behavior

`CHEAT_EFFECT_TRIGGER` is throttled per peer on the server:

- window: 1000 ms
- max events per window: 8
- events over the limit are dropped silently

This keeps cheat-trigger sync bursts from flooding peers while preserving expected world/player parity for testers.

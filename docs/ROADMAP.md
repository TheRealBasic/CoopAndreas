# CoopAndreas Roadmap

Detailed backlog extracted from `README.md` sections: **Current Tasks**, **TODO Launcher**, **TODO Missions**, and **TODO Other Scripts**.

Legend:
- **Priority:** `P0` = critical for playable co-op baseline, `P1` = important parity/quality, `P2` = optional or long-tail.
- **Effort:** `S` (small), `M` (medium), `L` (large).

## Milestone: Core Sync Stability

### P0 (must-have)
- [x] **Pickups sync** — collectibles, static pickups, and dropped pickups (`money`, `weapons`). **[P0][L]** ✅ Completed (2026-03-26).
  - **Acceptance criteria:** pickup state changes persist across reconnects for all connected peers and late joiners.
  - **Acceptance criteria:** collectible categories (graffiti/horseshoes/snapshots/oysters) do not duplicate or respawn incorrectly after host migration/rejoin.
  - **Acceptance criteria:** dropped money/weapons appear, can be collected once, and are removed consistently across at least 2 peers.
- [x] **Wanted level sync**. **[P0][M]** ✅ Completed (2026-03-25).
  - **Acceptance criteria:** wanted level changes are mirrored and verified across 2+ peers in free roam and mission contexts.
  - **Acceptance criteria:** reconnecting peer receives current wanted level without requiring a manual refresh.
- [x] **Passenger sync completion** (`gamepad support`, `radio sync`). **[P0][M]** ✅ Completed (2026-03-25).
  - **Acceptance criteria:** controller inputs map correctly for non-driver passenger actions on all clients.
  - **Acceptance criteria:** radio station selection/state stays consistent for driver + passengers across 2+ peers.
- [x] **Vehicle sync completion** (`force hydraulics sync`, `trailer sync`). **[P0][L]** ✅ Completed (2026-03-26).
  - **Acceptance criteria:** hydraulic state transitions replicate reliably without visual divergence.
  - **Acceptance criteria:** trailer attach/detach and trailer physics state remain consistent across 2+ peers over 5+ minutes.
- [x] **NPC sync completion** (`in vehicle: horn/siren`, `aim`, `shots`, `task sync`). **[P0][L]** ✅ Completed (2026-03-25).
  - **Acceptance criteria:** NPC in-vehicle horn/siren state changes replicate in real time for all peers.
  - **Acceptance criteria:** NPC aiming and firing targets/impact events match across 2+ peers.
  - **Acceptance criteria:** task/state machine replication avoids stuck or desynced behavior over mission-length sessions.

### P1 (important)
- [x] Jetpack pickup interaction (depends on pickup sync). **[P1][S]**
- [x] Player map sync: `Areas/GangZones`, map-pin proportion fix. **[P1][M]**
- [x] Stats sync: `breath level bar`, `max hp sync`. **[P1][M]**
- [x] Fire sync. **[P1][M]**
- [x] Cheat code sync. **[P1][M]**
- [x] Animation sync: `idle anims`, special `TAB+NUM4/NUM6` anims. **[P1][M]**
- [x] Gang groups sync. **[P1][M]**
- [x] Widescreen fix. **[P1][S]**
- [x] Smooth interpolation: movement + rotation. **[P1][M]**

### P2 (nice-to-have)
- [ ] Player voice commands. **[P2][S]**
- [ ] Chat reactions (`LD_CHAT.txd`). **[P2][S]**
- [ ] Gang wars sync. **[P2][L]**
- [ ] Parachute jump sync. **[P2][M]**
- [ ] Stunt systems (`collecting`, per-player slow motion). **[P2][M]**
- [ ] Chat gamepad support with on-screen keyboard. **[P2][M]**
- [ ] Minor ideas: sync laser sniper red dot, moon-sniper easter egg shot size effect. **[P2][S]**

## Milestone: Mission Parity

### P1
- [ ] Implement remaining storyline mission sync parity (all open missions from `Cleaning The Hood` through `End Of The Line`). **[P1][L]**
  - Scope includes all missions not already completed (`Big Smoke`, `Ryder`, `Tagging Up Turf` were already done and are intentionally excluded from open backlog).

## Milestone: Launcher UX

### P1
- [ ] Start/control server directly from launcher. **[P1][M]**

## Milestone: Optional Content

### P2
- [ ] Property purchase sync. **[P2][M]**
- [ ] Submissions: `Taxi`, `Firefighter`, `Vigilante`, `Paramedic`, `Pimp`, `Freight Train`. **[P2][L]**
- [ ] Hidden races: `BMX`, `NRG-500`, `Chiliad Challenge`. **[P2][M]**
- [ ] Stadium events: `8-Track`, `Blood Bowl`, `Dirt Track`, `Kick Start`. **[P2][M]**
- [ ] Ammu-Nation challenge. **[P2][S]**
- [ ] Schools: `Driving`, `Flight`, `Bike`, `Boat`. **[P2][L]**
- [ ] Asset missions: `Trucker`, `Valet`, `Career`. **[P2][L]**
- [ ] Courier sets: LS/SF/LV routes. **[P2][M]**
- [ ] Street Racing set (22 races). **[P2][L]**
- [ ] Import / Export. **[P2][M]**

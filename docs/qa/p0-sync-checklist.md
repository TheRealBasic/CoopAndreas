# P0 Sync QA Checklist (Multiplayer Repeatable Scenarios)

This checklist verifies the **P0 Core Sync Stability** acceptance criteria in `docs/ROADMAP.md` for:
- pickup sync (collectibles, static pickups, dropped `money`/`weapons`), and
- vehicle sync completion (`force hydraulics sync`, `trailer sync`).

> Recommended session size: **3 peers** (Host + Peer B + Peer C late join/rejoin).

---

## 1) Collectible pickup + rejoin/late-join matrix (2+ peers)

**Roadmap acceptance bullets covered**
- Pickup state changes persist across reconnects for all connected peers and late joiners.
- Collectible categories (graffiti/horseshoes/snapshots/oysters) do not duplicate or respawn incorrectly after host migration/rejoin.

### Setup
1. Start one co-op session with **Host + Peer B**.
2. Enable logging/recording for pickup state events (if available).
3. Pre-select one collectible from each category in accessible areas:
   - Graffiti tag
   - Horseshoe
   - Snapshot
   - Oyster
4. Record baseline: collectible present/absent state for each peer.

### Steps (repeatable cycle)
1. Host collects target collectible while Peer B observes at location.
2. Verify immediate removal/collected state on both peers.
3. Peer B disconnects and rejoins same session.
4. Verify collectible remains collected for Host + rejoined Peer B.
5. Add **Peer C** as late joiner; verify collectible is still collected.
6. Repeat steps 1–5 for all four collectible categories.
7. Perform one host migration/rehost cycle (if supported), then repeat one category sample.

### Expected network-visible outcomes
- Collected item state propagates to all connected peers in-session.
- Rejoining/late-joining peers receive authoritative collected state (no respawn/duplication).
- After host migration/rejoin, previously collected category items remain collected.

### Pass / Fail criteria
- **PASS**: Across all categories, no collectible reappears after reconnect/late join/host migration and no peer can recollect an already collected item.
- **FAIL**: Any peer sees collectible respawn, duplication, or can collect the same item more than once after sync events.

---

## 2) Static pickup persistence + peer rejoin (2+ peers)

**Roadmap acceptance bullets covered**
- Pickup state changes persist across reconnects for all connected peers and late joiners.

### Setup
1. Session with **Host + Peer B**.
2. Select at least 3 static world pickups in different zones/interiors (e.g., health/armor/utility).
3. Confirm all selected pickups are visible to both peers before test.

### Steps (repeatable cycle)
1. Peer B collects static pickup #1 while Host watches.
2. Confirm removal on both peers.
3. Host disconnects/rejoins.
4. Confirm static pickup #1 remains consumed for rejoined Host.
5. Add Peer C as late joiner and verify same state.
6. Repeat for pickups #2 and #3, alternating collecting peer (Host/Peer B).

### Expected network-visible outcomes
- Pickup consumption is globally reflected and persists through reconnects.
- Late joiners inherit already-consumed pickup state.

### Pass / Fail criteria
- **PASS**: All tested static pickups maintain correct consumed/available state for all peers before and after reconnect/late join.
- **FAIL**: Any consumed pickup is visible again incorrectly or has peer-specific divergent state.

---

## 3) Dropped weapon/money simultaneous pickup race

**Roadmap acceptance bullets covered**
- Dropped money/weapons appear, can be collected once, and are removed consistently across at least 2 peers.
- Pickup state changes persist across reconnects for all connected peers and late joiners.

### Setup
1. Session with **Host + Peer B** (add Peer C in verification phase).
2. Spawn one dropped money pickup and one dropped weapon pickup in open area.
3. Position Host and Peer B equidistant from target pickup.
4. Use countdown/voice sync to force near-simultaneous pickup attempts.

### Steps (repeatable cycle)
1. On countdown, Host and Peer B attempt to pick up dropped money at the same time.
2. Record winner, loser behavior, and world state on both clients.
3. Repeat 5 trials, swapping approach vectors.
4. Repeat steps 1–3 for dropped weapon pickup.
5. After each successful single-collector outcome, force loser to re-attempt pickup (should fail because removed).
6. Rejoin loser peer and confirm pickup remains removed.
7. Add Peer C late and confirm pickup absent.

### Expected network-visible outcomes
- Exactly one peer receives each dropped item in a race attempt.
- Pickup entity is removed for all peers immediately/near-immediately after authoritative collect.
- Rejoined and late-joined peers do not see already-collected dropped items.

### Pass / Fail criteria
- **PASS**: In every trial, only one collector is granted reward/item and all peers converge on removed state with no duplicate grants.
- **FAIL**: Dual rewards, lingering visible pickup for any peer, or rejoin/late join reintroducing removed dropped item.

---

## 4) Hydraulics toggle stress (force hydraulics sync)

**Roadmap acceptance bullets covered**
- Hydraulic state transitions replicate reliably without visual divergence.

### Setup
1. Session with **Host + Peer B** (optional Peer C observer).
2. Use same hydraulics-capable vehicle model for all peers.
3. Park in open flat area, then plan a moving segment through varied terrain.
4. Define stress profile:
   - idle rapid toggles,
   - low-speed toggles,
   - high-frequency alternating inputs,
   - toggles during collisions/bumps.

### Steps (repeatable cycle)
1. Host performs 30-second rapid toggle burst while stationary; Peer B observes suspension state.
2. Host drives at low speed for 2 minutes while toggling every 2-3 seconds.
3. Peer B takes over driver role and repeats steps 1-2.
4. Execute 1-minute high-frequency alternating hydraulics input (left/right/front/back patterns).
5. Introduce minor collisions/curbs while toggling; monitor for state snapping/desync.

### Expected network-visible outcomes
- Remote peers see same hydraulics transitions and end-state orientation as local driver.
- No prolonged divergence (stuck compressed/extended state, mismatched wheel/suspension posture).
- State recovers correctly after collisions and continued toggles.

### Pass / Fail criteria
- **PASS**: No persistent visual or state divergence longer than brief network jitter; final hydraulics posture matches across peers after each burst.
- **FAIL**: Reproducible mismatch in hydraulics state or prolonged divergence requiring reset/re-entry to resync.

---

## 5) Trailer attach/detach sync over 5+ minute drive session

**Roadmap acceptance bullets covered**
- Trailer attach/detach and trailer physics state remain consistent across 2+ peers over 5+ minutes.

### Setup
1. Session with **Host + Peer B** (Peer C optional observer/late join).
2. Select tractor + trailer combination with reliable spawn points.
3. Plan route lasting **at least 5 minutes** including turns, braking, reversing, and uneven surfaces.
4. Start synchronized timer when trailer is first attached.

### Steps (repeatable cycle)
1. Attach trailer as Host; Peer B confirms visual/physics attach.
2. Drive full 5+ minute route with varied maneuvers.
3. At minute ~2 and ~4, perform controlled detach and reattach events.
4. Swap driver to Peer B and continue 2+ additional minutes with one attach/detach cycle.
5. During one attached segment, add Peer C as late joiner and verify attach + trailer motion state.
6. Rejoin one existing peer mid-route and validate current attachment/physics state.

### Expected network-visible outcomes
- Attach/detach transitions occur consistently on all peers.
- Trailer transform/physics (position, orientation, sway, collision response) remain aligned throughout route.
- Rejoin/late-join peers receive current attachment state and trailer motion without reset artifacts.

### Pass / Fail criteria
- **PASS**: Over full timed run, all peers maintain consistent trailer attachment status and physics behavior with no persistent desync.
- **FAIL**: Any peer sees ghost trailer, mismatched attach status, major transform drift, or invalid physics after attach/detach/rejoin.

---

## Execution notes (repeatability controls)

- Run each scenario for **minimum 3 iterations** on separate session restarts.
- Rotate who is host and who performs pickup/vehicle actions.
- Capture timestamped evidence (video/log snippets) for each failure.
- Mark scenario result as **Pass**, **Fail**, or **Blocked** (environment/setup issue only).

## Sign-off template

| Scenario | Iterations | Result | Evidence Link | Notes |
|---|---:|---|---|---|
| Collectible pickup + rejoin/late-join matrix | 3+ |  |  |  |
| Static pickup persistence + peer rejoin | 3+ |  |  |  |
| Dropped weapon/money simultaneous pickup race | 3+ |  |  |  |
| Hydraulics toggle stress | 3+ |  |  |  |
| Trailer attach/detach over 5+ minute drive | 3+ |  |  |  |

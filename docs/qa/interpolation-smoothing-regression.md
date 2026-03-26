# QA: Interpolation Smoothing and Snap Regression Scenarios

## Scope
Validate remote actor interpolation changes for:
- split translation vs rotation smoothing,
- dead-reckoning micro-jitter suppression,
- hard-snap fallback on large divergence,
- remote-only interpolation application.

## Scenario 1: On-foot sprint (remote player)

### Setup
1. Start two clients connected to the same server.
2. Keep Client A stationary and observe Client B as a **remote** actor.
3. On Client B, sprint in zig-zag patterns with frequent direction changes.

### Steps
1. Record 30 seconds of remote movement from Client A perspective.
2. Repeat under moderate packet delay/loss simulation if available.
3. Compare with baseline build if available.

### Expected
- Remote position remains smooth without visible micro-jitter at near-constant pace.
- Heading/aiming updates avoid abrupt snaps during ordinary direction changes.
- Large divergence events (teleport/respawn) resolve in one hard snap, not long drift.
- Local player on each client is unaffected by interpolation corrections.

## Scenario 2: High-speed vehicle turns (remote vehicle)

### Setup
1. Spawn a fast vehicle (e.g., Infernus/Cheetah equivalent).
2. Client B drives repeated high-speed S-curves while Client A spectates remotely.

### Steps
1. Perform at least 5 high-speed turn sequences.
2. Include one intentional desync event (forced reset/respawn/teleport if admin tools exist).
3. Observe correction behavior for both position and orientation.

### Expected
- Vehicle body rotation follows turns continuously with reduced heading snap artifacts.
- Position path remains stable without over-smoothed lagging behind corners.
- Minor packet-to-packet deltas do not produce camera-visible jitter.
- Major divergence triggers immediate snap to authoritative state.

## Scenario 3: Trailer-linked rotation continuity

### Setup
1. Attach trailer to a tractor vehicle on Client B.
2. Observe from Client A as remote-synced tractor+trailer pair.

### Steps
1. Perform wide and tight turns at low and medium speed.
2. Detach/reattach trailer once during movement.
3. Continue turning after link-state transition.

### Expected
- Tractor and trailer orientation updates remain continuous through turns.
- No prolonged rotational drift after attach/detach transitions.
- Hard snap occurs only on clear large divergence, not on small linkage adjustments.
- No interpolation applied to locally controlled vehicle on the driving client.

# Gang-war lifecycle parity checklist

## Setup
1. Start one host and at least two clients.
2. Ensure one client joins after a gang war has already started.
3. Keep one client ready to disconnect/reconnect during an active gang war.

## Start/finish parity
1. On host, trigger a gang war in a turf zone.
2. Verify all clients observe the same gang-war start moment (state transition from idle to active).
3. Continue until war finishes and verify all peers converge on the same outcome (win/loss).
4. Repeat at least once with players in different `currArea` values (interior transition before/after trigger).

## Wave progression parity
1. During an active war, progress through at least two wave transitions.
2. Verify each wave progression event is observed on all peers in the same order.
3. Confirm no duplicate or out-of-order lifecycle events are applied on clients.

## Reconnect and late-join parity
1. Disconnect a client mid-war, then reconnect before outcome.
2. Verify reconnecting client receives the host-authoritative lifecycle snapshot and resumes the current phase without manual refresh.
3. Join an additional late client after war ends and verify the latest lifecycle event (including outcome) is immediately present.

## Zone ownership persistence parity
1. Complete a war that changes turf ownership.
2. Verify zone owner/color/state values match across all connected peers.
3. Disconnect and reconnect a client; verify ownership state persists after snapshot replay.
4. Start a new war in another zone and confirm previous ownership data remains consistent.

## Pass criteria
- Host-auth lifecycle events are ordered and consistent across peers.
- Start, wave progression, and finish outcomes match for all peers.
- Rejoiners and late joiners receive the latest lifecycle snapshot without desync.
- Turf ownership changes persist across reconnects and subsequent wars.

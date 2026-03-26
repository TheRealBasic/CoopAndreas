# Map-state parity checklist (gang zones + pins)

## Setup
1. Start one host and at least two clients.
2. Use at least two different display aspect ratios (example: 16:9 and 21:9).
3. Ensure at least one client joins late (after state changes already happened).

## Gang-zone owner/color parity
1. On the host, trigger at least three gang-zone ownership/state changes (different owners/colors/states).
2. Verify each non-host client shows identical owner/color/state for each changed zone.
3. Have a late-join client connect and verify it receives the same zone owner/color/state snapshot immediately.
4. Repeat after one host interior transition to confirm zone state updates continue while `currArea` changes.

## Map-pin placement parity
1. Place at least three map pins (far west edge, far east edge, central city).
2. Verify all peers render the same pin world position (no edge drift on ultrawide).
3. Clear one pin and place another while changing interior/area.
4. Have a late-join client connect and verify active pins match existing peers.
5. Validate pins remain clamped to world/radar bounds (no out-of-range coordinates).

## Pass criteria
- Gang-zone owner/color/state values match across all peers.
- Late joiners get current zones and active map pins without manual refresh.
- Aspect ratio differences do not change pin placement.
- Interior transitions do not pause/suppress map-state updates.

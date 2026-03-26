# Property purchase sync parity checklist

## Script inventory (buy points + ownership flags)

### Ownership flags observed in `scm/scripts/*.txt`
- `$already_bought_house[0..31]` initialization/reset in `INITIAL.txt` (32 property ownership bits).  
- `$zeros_property_bought` set in `BUYPRO1.txt` and checked in `MOB_SF.txt` for Zero's asset progression gating.

### Property buy pickup slots observed in scripts
- `save_housepickup[0..31]` is the canonical buy/save pickup array.
- Initial state (`INITIAL.txt` + `MAIN.txt`): slots `0..2` are locked asset properties, and mixed locked/for-sale setup for `3..31`.
- Unlock promotions (`OPENUP.txt`): converts additional locked slots to `Pickup.CreateForSaleProperty('PROP_3', ...)`.
- Entry/exit linkage is managed with `World.SetClosestEntryExitFlag(...)` calls tied to the same `propertyX/Y/Z` slots.

## Test matrix

### 1) Purchase propagation (host-authoritative)
1. Start with two peers connected and ensure both can see the same for-sale marker for one target slot.
2. Host purchases the property.
3. Verify non-host immediately observes:
   - ownership state changed,
   - pickup/interior linkage state updated (for-sale marker removed/converted),
   - no local rollback after a few seconds.
4. Repeat with a second property in a different area/interior.

### 2) Rejoin parity
1. After at least two purchases, disconnect non-host.
2. Continue gameplay on host for 30+ seconds.
3. Reconnect non-host.
4. Verify reconnect snapshot restores identical ownership/unlock/pickup-interior linkage state.

### 3) Late-join parity
1. Keep existing session running after purchases.
2. Join with a fresh third peer (no prior state).
3. Verify late joiner receives full property snapshot immediately on connect (no temporary for-sale rollback visuals).

### 4) Host-migration parity
1. Purchase at least one property.
2. Force host migration (original host leaves).
3. Verify remaining peers keep identical property ownership state post-migration.
4. Verify newly assigned host can continue updates without reverting previously purchased properties.

## Pass/Fail
- **PASS:** ownership flags, unlocked state, and linked pickup/interior state remain identical across connected peers, rejoiners, late joiners, and post-migration peers.
- **FAIL:** any purchased property reverts to for-sale/locked for any peer, or diverges after reconnect/host migration.

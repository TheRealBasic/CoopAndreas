# Networked mission widescreen precedence

Expected behavior for `switch_widescreen` (`0x02A3`) on non-host peers:

1. **Mission/cutscene forced state wins while active.**
   - During an active mission or cutscene sync window, the latest mission-script widescreen toggle is treated as authoritative for that event.
2. **Apply once per event transition.**
   - Duplicate toggles with the same value in the same mission/cutscene event are ignored client-side to avoid reapplying every frame.
3. **User preference is restored after mission/cutscene completion.**
   - When mission flag clears or cutscene finishes/clears, forced widescreen override is dropped and the saved local preference is restored.
4. **Stale packets are ignored.**
   - If `switch_widescreen` arrives after mission/cutscene completion, it is ignored as stale and does not change local camera mode.

This preserves mission-script intent during synchronized scenes while preventing persistent override of local graphics preference after synchronized content ends.

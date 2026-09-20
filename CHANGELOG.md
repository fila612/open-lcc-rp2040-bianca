# Changelog

## 0.5.0 — 2026-09-20

Changes since v.0.4.0. Includes the fixes previously developed locally as 0.4.1;
no separate 0.4.1 release was published.

### Behavior

- Require lowering and raising the brew lever after a start was rejected for missing
  operational readiness. Reaching readiness while the lever remains raised does not
  start a shot. This also applies when reheating after sleep.
- Report an accepted shot through the existing `currentlyBrewing` wire field.
  Pump-off pre-infusion remains part of the shot; lever release or a bail clears
  the reported state. Existing internal lever-driven wake/automation behavior stays intact.
- Reject manual and automatic sleep during cold-start heat-up. Preserve startup
  restoration of sleep and reset the readiness stability timer on sleep/wake.
  An expired auto-sleep countdown can take effect immediately after heat-up ends,
  before operational readiness is reached.

### Fixes

- Limit the JEDEC flash-ID transfer to its four-byte buffer, fixing a stack overrun.
- Cancel a pending Core 1 restart when its status queue recovers. Continuous stalls
  retain the two-second initial timeout and five-second retries.

### Compatibility and unchanged behavior

- Status field order, types and packet size are unchanged from 0.4.0. No extra
  Brew Active entity or new protocol negotiation is introduced.
- Older ESPHome versions that accept the 0.4.0 layout can read this release, although
  their own legacy display/counting rules still apply. ESPHome 0.10.0 recognizes
  accepted-shot semantics using the existing version bytes. No new update order is required.
- Tank-low during an accepted shot still permits that shot to finish; new shots
  remain subject to the tank-empty latch. The existing error recovery policy,
  heater settings and pre-infusion timing are unchanged.

### Validation

- Seven host regression scenarios passed with AddressSanitizer/UndefinedBehaviorSanitizer.
- RP2040 firmware build passed; status layout compared with v.0.4.0.
- Hardware validation remains outstanding. This release has not been flashed as
  part of its preparation.

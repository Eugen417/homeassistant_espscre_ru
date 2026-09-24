# Page-owned layout review status

This is a work-in-progress review checklist, not release acceptance. Product
decisions retain independent page top bars, free map positions and the 1 by 2
and 2 by 2 tile sizes in this release. Pagination uses a positive, enabled by
default checkbox. No changes have been published.

## Numbered review items

| Item | Implemented | Still open |
| --- | --- | --- |
| 1. Taller-tile scope | Sizes remain in 0.3.1 as selected; the feature has a separate commit. | No branch split is requested. |
| 2. Migration | Historical grid fallback verified against Git history; per-tile recovery, unknown option filtering, NaN isolation, original backup, dismissible dropped-tile notes, real startup-order tests. | Recovery for malformed page metadata and dangling links; pending-layout delivery and explicit Start fresh. |
| 3. Upgrade communication | Easy Setup and changelog explain update order, backup, downgrade and old editor tabs. | Complete firmware translations. |
| 4. Translations | New editor messages have English and Dutch keys. | Complete other languages, remove user-facing literal errors and enforce missing keys. |
| 5. Firmware safety | Checked record reservations replace the aborting allocator. Failed reservations retain the old model and transfer; equal-size replacements reuse storage. Navigation no longer requires HA connectivity. | All-board host regression with disconnected HA and refusal scenarios; remove obsolete packing path; physical CYD acceptance. |
| 6. Editor defects | Explicit conflict recovery, offline capability retention during a connection outage, removal-toast expiry, grouped text undo, destination-title preservation. | Broader UI acceptance and capability retention across add-on restarts. |
| 7. Page top bars | Independent page bars retained as selected. | Negotiate deduplicated state delivery for shared entity references. |
| 8. Pagination wording | Positive checkbox, default on; English/Dutch and page documentation updated. | Other translations. |
| 9. Editor modes | Free map positions retained as selected. Simple offers navigation preview; unavailable duplication is hidden; empty copy is prominent. | Isolate map-history actions from Simple-mode undo. |

Additional work still includes receiver/service extraction, cosmetic in-place
updates, a shared size capability table, old-editor revision enforcement,
conformance fixtures and editor model cleanup. The compiled-layout read cache
now uses the file stamp, and navigation logging only reports an accepted move.

## Verification completed during this review

- Full fast checks: 702 Python tests, 25 C++ programs and 209 editor tests, plus
  type checking, generated files and the editor build.
- Address/undefined-behavior sanitizers on the runtime model and page protocol.
  Fault injection refuses either record allocation and checks that the original
  model, record addresses and active state survive.
- All six firmware variants compile with ESPHome 2026.9.0. CYD is 1,631,888 B,
  leaving 203,120 B in its OTA slot. Growth against the matching 0.2.104 baseline
  is 3,424 B; this is 912 B smaller than the preceding 0.3.1 review build.
- All five eligible profiles also compile with minimum ESPHome 2026.6.2. CYD
  uses 1,657,152 B (90.3%), leaving 177,856 B, with 3,296 B growth against its
  matching baseline. The existing tight-flash warning remains. The P4 requires
  ESPHome 2026.8.0 and is excluded from this minimum-version run.
- CYD host rendering passes 40 page checks. This is simulated input, not a
  physical touchscreen test and not yet a disconnected-HA swipe test.
- The disposable Guition was flashed and its compilation identity verified.
  Its unchanged eight-tile layout passed ten geometry checks and fifty overlay
  render cycles. No physical taps or household device actions were performed.

## CYD physical acceptance, pending an available test board

Use a disposable CYD with a backed-up layout and a test Home Assistant instance.
Do not run this on a household control panel without its owner's approval.

1. Load eight pages, each with a top bar. Include ordinary and taller tiles,
   navigation links and a detail page excluded from dots and swipes. Check the
   saved revision is applied before starting the measurement.
2. Capture the `health` log values `free_heap`, `largest_block` and `min_free`
   for at least 20 minutes. Record the starting, lowest and final readings and
   any resets. Compare settled readings after equivalent workloads, not the
   transient allocation peak during a save.
3. Physically swipe between included pages, open a detail page through its link
   and return using the bottom bar. Hide the bottom bar and repeat using the
   header Back control. Check that neighboring tiles keep their dimensions.
4. Restart only the test HA instance while continuing physical swipes. Page
   navigation must keep working; unavailable entity actions must not run.
5. With an operator sending a deliberately out-of-range layout transaction,
   confirm refusal leaves the current pages visible and usable. This checks
   protocol bounds, not actual allocator exhaustion; injected allocator failure
   is covered separately by the native tests.
6. Reconnect HA, wait for the saved revision to be acknowledged, and repeat
   navigation and a same-size save. Record the total observed uptime and attach
   the log. Do not infer touch acceptance from compilation or rendered images.

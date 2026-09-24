# Response to the page-owned layout review

The review fixes retain stable page IDs, one canonical page document and a single
firmware configuration. No changes have been published. The detailed checklist
and hardware acceptance steps are in [PAGE_REVIEW_STATUS.md](PAGE_REVIEW_STATUS.md).

## Product decisions

Pages retain independent top bars and freely draggable map positions. The positive
"Show in page dots and swipes" option is enabled by default. Taller 1 by 2 and
2 by 2 tiles remain in this release, with their original separate feature commit.
These choices intentionally differ from the review's proposed default-bar model,
automatic map layout and separate release branch.

## Changes to inspect

- **Migration and recovery:** historical-grid detection, partial recovery of valid
  tiles and metadata, preserved link destinations, original-file backup and
  dismissible recovery notices. Unknown data cannot silently overwrite a current
  document. Migration happens in the add-on; firmware has no legacy decoder.
- **Delivery and memory:** checked allocations retain the active model on failure.
  Shared resolved header values use negotiated `bar_values`. Negotiated
  `appearance_updates` changes titles, tile labels and backgrounds without replacing
  the model or closing a detail card. Unsupported peers use the existing transaction.
  Both receivers validate the complete packet before applying changes.
- **Firmware structure and navigation:** the receiver now lives in
  `page_receiver.h`. Swipe paths share one destination check. Local navigation
  works without HA, and rejected navigation is not logged as an accepted move.
  Back uses the footer, or the full-size Home position when the footer is hidden.
- **Add-on structure:** page save and synchronization policy moved into
  `page_service.py`. File-stamp and registry-identity caches avoid repeated work.
  Offline capability hints survive restarts but cannot establish a live protocol
  session or authorize an unverified replacement. Old editor saves are refused.
- **Editor correctness:** explicit conflict recovery, grouped text undo, expired
  removal toasts and scoped map history. Selection uses page IDs. History, workspace
  persistence and conflict handling have separate modules. In-flight map saves
  cannot swallow a pending save after switching screens. Grid operations read the
  current document directly, without a global `setGrid` watcher.
- **Shared validation and language:** Python and TypeScript run the same 52
  conformance documents. Generated card rules reduce drift. Page, storage and
  delivery errors are translated, including cached errors read in another language.
- **Additional requested UX:** compact page creation with title, Home, clock and
  entity selection; one-time room/domain naming when a drop creates a page;
  contextual Floating Vue help with focus, tap and Escape support. Important
  status and error messages remain visible.

## Verification and remaining limits

The full check passes: 724 Python tests, 25 C++ programs, 275 editor tests,
translations, generated files, type checking and the committed frontend build.
Firmware builds pass for all six current profiles and all five profiles eligible
for the minimum ESPHome version. The ten host variants pass 380 page checks,
including disconnected navigation and atomic refusal. Additional appearance checks
keep the model and open-card pointers unchanged. CYD uses 1,634,352 bytes with the
current compiler, leaving 200,656 bytes in its OTA slot.

Real Ingress browser checks cover wizard cancellation, configured page creation,
one-step undo, tap/focus help, Escape and a narrow-screen layout. The test-HA
restart retained configuration and the board acknowledged the saved revision.

The disposable Guition runs the review firmware and passed ten geometry checks
and fifty overlay render cycles. These are software-driven checks, not physical
touch acceptance. CYD glass and sustained-memory acceptance still need an available
test board and operator, as listed in the detailed checklist.

Tile dragging still uses a checked slot projection. Every existing tile ID must
be returned, preventing the silent deletion identified in the review. A complete
move to direct placement editing remains a later internal cleanup.

# Pages, top bars and navigation

ESP Screens 0.3.0 stores each page as a complete unit: its tiles, top bar, navigation choice and stable identity. Moving a page keeps links pointing to that page. A page's number is its current position in the editor, not its identity.

## Editing

**Simple** is the default. Drag entities from the library onto the page grid, or select an empty cell and add an entity. Page settings contain the title, top-bar items, Home designation and **Show in page dots and swipe navigation**. Every page can have different top-bar items. A title can follow the screen title or use custom text. Copying a bar creates independent items on the chosen pages.

**Advanced** adds a spatial page map and links from navigation tiles to their destinations. Workspace positions belong to the editor, so arranging pages vertically does not change their order on the device. Reordering pages changes the device order while preserving destinations. Returning to Simple removes the map connections and keeps the same pages and tiles.

The navigation preview runs locally. It does not switch entities in Home Assistant. Entity values and available history come from Home Assistant; unavailable history is left empty.

Explicitly adding a page opens a compact setup dialog for its title, Home control,
clock and optional Home Assistant entities. Cancelling leaves the document unchanged;
creating the page is one undo step. Dragging a tile onto a new page skips the dialog.
The new page gets a title from its shared HA area, or from its entity domain when
there is no shared area. Mixed content without a common area uses the screen title.
Suggestions are applied only at creation and never rename an existing page.

Contextual help uses Floating Vue through `HelpTip.vue`. Help icons work with
keyboard focus, pointer hover and a tap; Escape or leaving the help closes it.
Errors, compatibility notices and unsaved-change status remain visible.

## Home and detail pages

Choose any page as Home. Home controls, automatic return and return on standby use that destination. Existing Home settings and entity identifiers stay compatible. Home Assistant entities whose names still say "Back to page 1" now return to the configured Home page; their existing names and IDs are retained so automations keep working.

Every page initially participates in the bottom paginator and sequential swipes. Turn off **Show in page dots and swipe navigation** to opt a page out. Links and the existing numeric Show page action can still open it. Numeric actions follow the current editor order; links stored in the layout follow stable page IDs.

An excluded page is a detail page:

- With the bottom bar enabled, that strip shows **Back**. It reserves the same space on all pages, so navigating does not resize tiles.
- With the bottom bar hidden, a left chevron replaces the top-bar Home control at the same visible height, with the same touch target and title position. Back works even when the ordinary Home control is disabled.
- Back follows the route used to reach the page. Nested routes keep at most eight page IDs. Deleted destinations are skipped, and an entry without history falls back to Home.
- Home clears that route. Sequential swipes stay within included pages and do not turn a detail page into a shortcut through the paginator.

A single-page layout retains the existing absence of a bottom strip. Device grid dimensions and tile capacity remain authoritative. They cannot be overridden per page.

## Taller tiles, 0.3.1

The developer editor offers **1 × 2** and **2 × 2**, expressed as width × height in grid cells, when the connected firmware reports support. Set `SCREEN_EDITOR_ENV=development` in the add-on process environment and restart it to enable these experimental choices. They are hidden by default while their designs are being refined. Existing saved taller tiles remain intact when the flag is removed. The firmware and document format do not depend on this editor flag.

Each size reserves two rows, including their normal gap. Tiles cannot overlap or extend beyond a page, and the screen's tile capacity does not change. If there is no free rectangle during resizing, the previous size and position stay intact.

Hover over a tile or focus its edge handle to resize it. The right handle changes width; the bottom handle changes height and only appears in the developer editor. Handles offer only supported sizes that fit at the current position without moving neighbours. Drag to preview, release to apply, or press Escape to cancel. Arrow keys work on a focused handle. Each completed resize is one undo step. Full-page cards retain their existing inspector setting and do not have edge handles.

These sizes reuse existing designs. A 1 × 2 tile keeps the single-column design, including its optional slider or graph. A 2 × 2 tile keeps the double-width design and its direct controls. Extra height does not enable a different control set or force a full-page design. Moving, copying, exporting and undoing keep the rectangular footprint with the tile.

Taller standard tiles extend the existing header. Media uses the selected playback or volume controls below track information. Selecting **Album cover** uses the artwork as a dimmed background on boards that support pictures. Climate can put the target between round minus and plus controls, or show the measured temperature above selected HVAC modes. Light brightness and other sliders reuse the existing large controls. Unavailable entities keep their unavailable state and their detail action.

Controls remain an explicit choice in the tile inspector. Increasing height preserves a previously selected group and does not enable a default group on a previously unconfigured tile. There is one selected group per tile; additional height alone does not combine playback with volume or setpoint with modes. All single-row tiles and existing full-page designs keep their original renderer.

The layout measures the available content rectangle, active fonts and physical touch sizes. Optional text gives way before touch targets. A control group that cannot fit an unusually dense custom grid is left in the detail view instead of drawing overlapping buttons. Source artwork is cropped, dimmed and rounded by the add-on, then decoded into the screen's existing shared image buffer. It does not allocate an additional image per tile. The atlas is bounded by the reported screen canvas; a missing or changed picture returns to the normal tile palette. The editor fetches prepared pixels through its relative Ingress API, never a Home Assistant token or source URL.

Update the screen before choosing a taller size. Its supported sizes are negotiated, separately from the page protocol. The add-on checks them before starting a replacement, so a saved rectangle cannot be silently reduced on an older screen. The storage version and migration path are unchanged.

Before downgrading to 0.3.0, change taller tiles back to Normal, Double width or Full page and save. Older add-ons cannot read the new presentations, even though the surrounding page-document structure is unchanged.

## Updating at different times

Update the add-on and screens in either order. Screens are negotiated individually.

| Add-on | Screen | Behaviour |
| --- | --- | --- |
| Older | Older | Existing behaviour |
| 0.3.0 | Older | Compatible layouts keep working through the add-on's older wire format. The editor asks to update that screen before enabling independent bars, a different Home target or pagination exclusions. |
| Older | 0.3.0 | The screen displays a configuration problem asking to update the add-on. New firmware carries no old layout decoder. |
| 0.3.0 | 0.3.0 | The latest saved layout is sent and activated automatically. No second Save or activation button is required. |

An unsaved editor draft remains a draft during an update. Updating one screen does not change the capabilities of another. A firmware downgrade cannot represent features its version never supported; the saved document is retained instead of being flattened.

The add-on checks the saved layout before installing new firmware. An external firmware update cannot perform that preflight. Missing grid information or invalid old references are shown as specific migration problems and must be resolved before that layout can activate.

## Storage and recovery

Layouts live in the add-on's `/data/screens.json`. Wi-Fi, API and OTA keys remain in the screen's ESPHome configuration. Layout exports contain no connection credentials.

On opening a version 1 store, the add-on creates a durable `screens.v1.backup.json` before replacing the store. Each convertible screen receives its stable IDs once. A screen whose source grid or data cannot be established safely keeps its original payload in a pending record. Other screens can still migrate and be edited. Pending records are retried when relevant screen/profile information changes, without requiring Save.

Normal saves write only version 2. State changes never rerun storage migration. Saves compare revisions and use a shared file lock, atomic replacement and durability checks. A stale editor receives a conflict instead of overwriting a newer edit. A failed send leaves a successful save intact for automatic retry.

Migration keeps valid tiles when individual tiles or old page settings are
invalid. The editor lists dropped tiles and settings replaced with defaults;
dismissing that note leaves the original backup intact. A readable pending
layout can still be sent to verified old firmware. For unreadable data,
**Start fresh for this screen** creates one empty page after confirmation,
retains the backup, and leaves other screens unchanged.

Top-bar value updates negotiate the `bar_values` capability. Identical rendered
values share one `bar_value` packet with explicit destinations, encoded as
`page index * 6 + item index`. The receiver validates all destinations before
changing any item. There are at most 48 destinations and no additional stored
firmware configuration. Different formatting remains independent; a change in
the number of visible items replaces that page's bar. Peers without the
capability continue receiving individual `bar` messages.

Title, label and background-only saves negotiate `appearance_updates`. The
add-on compares the remaining configuration before choosing one atomic
`appearance` packet, bound to the current revision. The receiver checks every
index and value before updating the existing records. Open cards, page history
and tile storage remain in place. Structural changes, older peers and edits
that exceed one bounded packet use the full transaction. No second layout or
compatibility decoder is stored on the screen.

Keep the pre-upgrade backup if an older add-on must be restored. Older add-ons cannot read version 2. Stop the add-on before restoring a backup; restoring it deliberately discards changes made after that backup. Unknown future storage versions are refused without rewriting them.

## Code boundaries and later features

| Responsibility | Source |
| --- | --- |
| Validated page document, stable references and compiled card inputs | `screen_manager/app/page_layout.py` |
| Historical storage/export conversion only | `screen_manager/app/layout_migrations.py` |
| Durable storage, backups, revisions and editor workspace | `screen_manager/app/layout_store.py` |
| Acknowledged device delivery and stale-response protection | `screen_manager/app/page_delivery.py` |
| Page save policy and capability checks | `screen_manager/app/page_service.py` |
| Editor history, workspace persistence and conflict recovery | `web/src/model/draft-history.ts`, `page-workspace.ts`, `page-conflict.ts` |
| Editor page operations and preview navigation | `web/src/model/pages.ts` |
| Firmware page metadata, navigation history and transfer guards | `components/smart_display/page_protocol.h` |
| Firmware transaction and appearance receiver | `components/smart_display/page_receiver.h` |
| Header widgets with explicit view data and a guarded action callback | `components/smart_display/page_header.h` |

The firmware keeps one active configuration. A structural replacement checks its required record reservations before releasing old storage; equal-size replacements reuse it. A failed reservation leaves the old model usable. A replacement becomes usable only when complete. Acknowledged sessions and revisions reject stale values and delayed responses. Pages and tiles share the existing PSRAM-first allocator; boards without PSRAM use their internal heap. The small Back history holds IDs only.

Editor placement helpers use a document-owned `createLayout` instance. They read the current document grid synchronously, without a shared mutable grid or watcher. The slot view remains a rendering and drag adapter; an arrangement must include every existing tile ID.

Tile placement has row, column, row span and column span, separated from content, appearance and interaction. Version 0.3.1 adds 1×2 and 2×2 to the permitted sizes. A future 2×3 climate design can extend the negotiated rendering capability without replacing the page model. A full-page card still means the whole current grid, even when its dimensions match another presentation.

Historical readers and the older wire adapter are separate add-on concerns. They can be retired independently after documenting a minimum supported source version and an intermediate upgrade or offline conversion route. Removing them never requires keeping migration machinery in firmware or changing current documents.

See the [0.3.0 test results](TEST_RESULTS_030.md) for upgrade checks, board builds, measured memory use and the limits of physical validation.
The [0.3.1 test results](TEST_RESULTS_031.md) cover taller tiles and their rendering and compatibility checks.

Responsive tall controls and their validation are described in
[the acceptance report](TEST_RESULTS_TALL_CONTROLS.md).

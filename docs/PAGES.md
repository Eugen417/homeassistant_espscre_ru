# Pages, top bars and navigation

ESP Screens 0.3.0 stores each page as a complete unit: its tiles, top bar, navigation choice and stable identity. Moving a page keeps links pointing to that page. A page's number is its current position in the editor, not its identity.

## Editing

**Simple** is the default. Drag entities from the library onto the page grid, or select an empty cell and add an entity. Page settings contain the title, top-bar items, Home designation and **Do not show in page navigation**. Every page can have different top-bar items. A title can follow the screen title or use custom text. Copying a bar creates independent items on the chosen pages.

**Advanced** adds a spatial page map and links from navigation tiles to their destinations. Workspace positions belong to the editor, so arranging pages vertically does not change their order on the device. Reordering pages changes the device order while preserving destinations. Returning to Simple removes the map connections and keeps the same pages and tiles.

The navigation preview runs locally. It does not switch entities in Home Assistant. Entity values and available history come from Home Assistant; unavailable history is left empty.

## Home and detail pages

Choose any page as Home. Home controls, automatic return and return on standby use that destination. Existing Home settings and entity identifiers stay compatible. Home Assistant entities whose names still say "Back to page 1" now return to the configured Home page; their existing names and IDs are retained so automations keep working.

Every page initially participates in the bottom paginator and sequential swipes. **Do not show in page navigation** opts a page out. Links and the existing numeric Show page action can still open it. Numeric actions follow the current editor order; links stored in the layout follow stable page IDs.

An excluded page is a detail page:

- With the bottom bar enabled, that strip shows **Back**. It reserves the same space on all pages, so navigating does not resize tiles.
- With the bottom bar hidden, a left chevron replaces the top-bar Home control. Back works even when the ordinary Home control is disabled.
- Back follows the route used to reach the page. Nested routes keep at most eight page IDs. Deleted destinations are skipped, and an entry without history falls back to Home.
- Home clears that route. Sequential swipes stay within included pages and do not turn a detail page into a shortcut through the paginator.

A single-page layout retains the existing absence of a bottom strip. Device grid dimensions and tile capacity remain authoritative. They cannot be overridden per page.

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

Keep the pre-upgrade backup if an older add-on must be restored. Older add-ons cannot read version 2. Stop the add-on before restoring a backup; restoring it deliberately discards changes made after that backup. Unknown future storage versions are refused without rewriting them.

## Code boundaries and later features

| Responsibility | Source |
| --- | --- |
| Validated page document, stable references and compiled card inputs | `screen_manager/app/page_layout.py` |
| Historical storage/export conversion only | `screen_manager/app/layout_migrations.py` |
| Durable storage, backups, revisions and editor workspace | `screen_manager/app/layout_store.py` |
| Acknowledged device delivery and stale-response protection | `screen_manager/app/page_delivery.py` |
| Editor page operations and preview navigation | `web/src/model/pages.ts` |
| Firmware page metadata, navigation history and transfer guards | `components/smart_display/page_protocol.h` |
| Header widgets with explicit view data and a guarded action callback | `components/smart_display/page_header.h` |

The firmware keeps one configuration. A replacement releases old records before allocating new ones and becomes usable only when complete. Acknowledged sessions and revisions reject stale values and delayed responses. Pages and tiles share the existing PSRAM-first allocator; boards without PSRAM use their internal heap. The small Back history holds IDs only.

Tile placement already has row, column, row span and column span, separated from content, appearance and interaction. Today the validator permits only existing renderable sizes. A future 2×2 player or 2×3 climate card can add a renderer capability and placement rules without replacing the page model. Those larger cards are not part of 0.3.0.

Historical readers and the older wire adapter are separate add-on concerns. They can be retired independently after documenting a minimum supported source version and an intermediate upgrade or offline conversion route. Removing them never requires keeping migration machinery in firmware or changing current documents.

See the [0.3.0 test results](TEST_RESULTS_030.md) for upgrade checks, board builds, measured memory use and the limits of physical validation.

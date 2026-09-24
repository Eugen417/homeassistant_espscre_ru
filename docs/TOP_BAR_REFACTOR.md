# Page-owned top bars: design contract

Status: implemented for 0.3.0, with release acceptance recorded separately. This document preserves the design decisions and acceptance requirements reviewed on 23 September 2026. See [Pages](PAGES.md) for current behaviour, source boundaries and upgrade rules.

Main reviewed and integrated through `8130856` (add-on 0.2.133), including the standby-override hotfix in `a40b3e4` (0.2.131), clipboard support in `803bb39` (0.2.132), and camera alerts addressed to selected screens. These changes remain covered by capability resolution, editor acceptance and protocol-2 camera delivery tests.

Give every page an identity and one complete top-bar configuration. Editing, moving, deleting, copying and restoring a page must operate on that configuration together with the page's tiles. The device should already hold every page's bar before the user navigates to it.

## Requirements and existing behaviour

- [Issue 30](https://github.com/MaxGramser/homeassistant_espscreen/issues/30) asks for independent items on each page, particularly to save tile space on small screens.
- [Issue 19](https://github.com/MaxGramser/homeassistant_espscreen/issues/19) introduced independent page titles. Preserve the choice to display the screen title and the ability to name page 1 independently.
- [Issue 26](https://github.com/MaxGramser/homeassistant_espscreen/issues/26) resulted in the Home button. Arbitrary page links in the bar remain deferred. This plan preserves Home and adds the separately specified per-page opt-out from sequential navigation.
- [Issue 27](https://github.com/MaxGramser/homeassistant_espscreen/issues/27) exposed two title problems: measuring LVGL's shortened label text, and measuring a new title one frame too late. Both fixes are acceptance requirements.

The configuration before 0.3.0 was split between `layout.header.items`, `layout.page_titles[]`, `layout.title`, and the device-owned `home_button` setting. The editor's `topbarItems()` reads a single global bar. The manager sends one `op: header` message, and firmware holds one `header_bar::Bar`. Page reordering separately remaps tile slots, navigation targets and title indexes.

The existing formatting, glyph handling, spacing, overflow rules, touch guard and local clocks are useful foundations. Retain them while changing ownership and delivery.

## Proposed model

Introduce an ordered collection of page records with stable IDs. Each record owns its top bar and tiles. Every page uses the grid provided by the board. Tiles and top-bar items also have stable instance IDs. Names below illustrate the contract; the referenced content, appearance and interaction types must be defined as validated variants for the features that already exist:

```typescript
type Page = {
  id: string;
  navigation: { excludeFromPagination: boolean }; // Opt-out, false for new and migrated pages.
  topbar: TopBar;
  tiles: TileInstance[];
};

type TopBar = {
  leading: HeaderControl[];
  title: { source: "screen" } | { source: "text"; text: string };
  trailing: HeaderItem[];
};

type TileInstance = {
  id: string;
  content: TileContent;
  placement: {
    row: number;
    column: number;
    columns: number; // Occupied columns, bounded by the board grid.
    rows: number; // Occupied rows, bounded by the board grid.
  };
  appearance: TileAppearance;
  interaction: TileInteraction;
};

type ScreenLayout = {
  title: string;
  homePageId: string;
  pages: Page[];
};

// Written by the manager from verified board information, not an editor setting.
type LayoutRecord = {
  sourceGrid: Readonly<{ columns: number; rows: number }>;
  layout: ScreenLayout;
};
```

Tile positions are local logical cells within their owning page. The board remains the sole authority for columns, rows, available tile cells, maximum tile count and maximum page count. There is no editable grid, cell-count override or per-page capacity setting in this design. The existing single, wide and full footprints consume that fixed grid as they do today. Keep the tested placement algorithms as pure functions with explicit board-grid input.

Store each tile's footprint as an explicit rectangle, not a closed list of size names. For this release, validation permits only the existing 1x1, wide and full-page footprints; the compiler maps those to the current renderer. Later 2x2 media cards or 2x3 climate cards need new rendering and placement capabilities, but no page-schema redesign. Rectangles must remain inside one page and cannot overlap other occupied cells. This is future support in the model, not additional size controls in this release.

Keep the renderer presentation in appearance, separately from that rectangle. `appearance.presentation` distinguishes the current `single`, `wide` and `full` cards; the compiler checks each presentation against its supported footprints. This matters already on one-column portrait screens, where a single tile and a wide control card both occupy 1x1 cells. Omitting the presentation means the default for the footprint. Adding a future 2x2 media presentation must not reinterpret an existing wide card merely because its geometry changed.

Preserve the actual capacity calculation, not a blanket promise of eight pages on every board: `maxPages = min(8, floor(64 / cellsPerPage))`. A six-cell grid currently supports eight pages and 48 occupied cells; a nine-cell grid supports seven pages and 63 occupied cells. Wide and full tiles consume their existing footprints within those cells. The limit of six below refers only to trailing top-bar items.

Persist a read-only snapshot of the source grid outside the editable layout, so an export or an offline layout can be interpreted without guessing the original board. The manager derives and verifies this metadata; a save cannot change hardware capabilities by supplying different metadata. On import, the source snapshot only explains the old positions. The destination board's grid and limits always govern what can be saved and sent. An incompatible layout needs an explicit adaptation within those limits or a clear refusal, never additional cells or silently discarded tiles.

A rebuild in another orientation or for another board can change that grid. Do not reinterpret saved row/column positions using the new column count or silently repack across page boundaries. Treat a changed grid as an explicit adaptation, preserving page IDs, Home and links and previewing any tile movement; refuse delivery until it fits. Normal supported rotation settings that leave the grid unchanged need no migration. Source-grid metadata describes the committed document until an adaptation is committed, not merely the latest reported hardware shape.

Flatten only at the firmware and legacy-API boundaries: `absolute_slot = page_index * cells_per_page + row * columns + column`. Firmware keeps compact tile records, page indexes, tile indexes and supported options. The new protocol may split their delivery into bounded records; the old serializer retains the existing messages. Keep a mapping from stable IDs to indexes for the active revision. The wire shape must not dictate how the editor stores and manipulates pages.

`pages` remains the complete ordered collection, including pages excluded from pagination. Derive the sequential navigation list by filtering out records whose `navigation.excludeFromPagination` is true, preserving the remaining order. Never filter the stored page collection, flattening indexes or numeric Show page mapping. This flag does not create extra capacity or a different page type.

Rules:

1. Each page owns its item array. Page 1 is an ordinary page. There is no global item array that individual pages override.
2. `source: screen` explicitly displays the screen title and follows a later rename. `source: text` displays that page's text. An empty custom title is normalized to `source: screen`; blank text does not create a second inheritance rule.
3. An empty item array intentionally displays no items. Missing configuration in an old stored layout is handled by the add-on's migration boundary, not a firmware renderer fallback. New firmware requires complete page configuration.
4. Keep up to six trailing top-bar items per page and the board's existing page and tile limits. Repeating an entity in different pages' top bars is valid. Preserve current duplicate-item rules within each bar. This does not enable duplicate entity tiles.
5. Keep current item types and their options: digital clock, analog clock, date, and entity state or last change, including icon and active-only visibility. Use a discriminated item type in the editor rather than unrestricted strings.
6. Model Home as a typed navigation control in `topbar.leading`, rather than a special boolean field on the bar. The first release supports zero or one leading Home control and up to six trailing information items, preserving the current geometry. Keep the existing Home setting, its Home Assistant entity and device preference as the device-wide enable switch. Actual visibility requires both a Home control on the page and that switch. Migrate every page with that control present. Home resolves to `ScreenLayout.homePageId`, which must identify exactly one existing page. Migrated layouts initially designate their first page; users can move that designation to any page in the editor.
7. Brightness, theme, language, clock format, standby, gestures and the settings long press remain device or application settings. They are inputs to drawing a page, not copied into each page record.
8. **Do not show in page navigation** is an opt-out, unchecked by default. New pages and migrated legacy pages have `navigation.excludeFromPagination: false`. Checking it excludes that page from paginator entries, previous/next navigation and sequential swipes, while keeping explicit page links, numeric Show page commands and Home routing available.

The collection replaces the stored numeric page count, global tile array and separate page-title array in the new schema. Screen identity and title remain separate from page identity. Do not keep writable copies of the legacy header, tiles and titles alongside the new model.

### Detail-page return navigation

An excluded page has a built-in Back control, without using a tile. When Page buttons is enabled, a multi-page layout reserves the same footer height on every page. Detail pages show Back there instead of arrows and dots. When the footer is hidden, detail pages replace the leading Home icon with a left chevron. This derived control is available even when the ordinary Home control is disabled; it does not add another saved top-bar field. Tile height stays constant across page changes in either setting.

Back follows at most eight stable page IDs held in a fixed buffer, without retaining old layouts. Page-link taps push their source, Back pops it and skips removed IDs, and reordering does not affect destinations. Home and external Show page commands clear the route. An empty history falls back to the designated Home. The editor simulator and reachability analysis use these same rules. Existing all-included layouts keep their ordinary navigation.

## Boundaries that make later changes smaller

Build these foundations in this refactor. They address assumptions already visible in the current code, rather than attempting to predict every future feature.

### Instance identity and references

An entity ID identifies a data source, not a visual object. A tile and a top-bar item each need their own stable ID even when they display the same entity. Preserve IDs on edits and moves, generate new ones on copies, and restore the original IDs on undo. Reject duplicate instance IDs during validation. Two instances can eventually show the same sensor as a value and a graph without becoming the same editable object.

The current firmware refuses repeated non-navigation tile entities. Instance IDs remove that assumption from the new document model, but do not by themselves enable duplicates on the device. Keep the current capability restriction until a separate firmware change audits state routing, actions, history and detail cards for duplicate entities. Legacy entity-addressed edit APIs must reject an ambiguous match if duplicates become supported; new edit APIs address instance IDs.

Page IDs must also be usable on the device to preserve the viewed page after reordering. Allocate unique opaque 64-bit page IDs, encode them losslessly as strings in JSON/TypeScript, and retain one compact key per configured page in firmware. Do not hash names or use ordinals as identity. Validate uniqueness, preserve IDs across saves and generate fresh IDs on copies/imports. Tile and header instance IDs remain in the manager/editor unless a separately justified device feature needs them.

Internal navigation targets use `{ kind: "page", pageId }`, rather than storing `screen.page_3` in the canonical model. The compatibility serializer translates them to the current numeric navigation entities. Existing external Show page commands remain ordinal, so an automation asking for page 3 still means the third page. Home is a separate target `{ kind: "home" }`, resolved through one function using `homePageId`. The Home button, bottom-edge gesture, automatic return, optional return on standby and explicit Go home command all use that resolver. Back uses the bounded page-ID history described above.

Pagination exclusion never changes those targets or ordinal meanings. On an included page, the physical paginator counts only included pages: five stored pages with two exclusions produce a three-page sequence. An excluded page shows no sequential counter or previous/next controls and does not respond to sequential swipes. It remains reachable through explicit navigation and offers built-in Back through the footer or the leading top-bar chevron. Never fabricate that history from page order. The Home designation is independent of the exclusion flag, including when Home itself is excluded. Zero or one included page produces no sequential controls, with no division by zero, looping search or implicit reinclusion; existing device settings can also suppress those controls.

Use one filtered previous/next resolver for physical arrows, swipe callbacks, paginator indicators and the simulator. Preserve the current stop-at-the-ends behaviour, with no implicit wrap. Sequential-control visibility is `active page included && included count > 1 && page_buttons`; swipes obey their own existing enable setting. Footer space is reserved for all pages when `page_count > 1 && page_buttons`. Hiding the footer in screen settings gives all pages the same taller tile area and moves detail Back into the top bar, without changing cell count or footprints. Explicit page targets always bypass this sequential resolver.

A page copy remaps references to itself to the new page and leaves references to other pages intact; copying the Home page does not make the copy Home. A whole-layout copy/import remaps every copied ID and internal reference together, including `homePageId`. Deletion follows the existing navigation-tile removal behaviour. If the Home page is deleted, the first remaining page becomes Home in the same operation, with that consequence shown in the editor; undo restores the deleted page and its Home designation. The final page cannot be deleted. No dangling page references may reach firmware.

Copy operations still obey duplicate-entity restrictions and capacity. In the first release, duplicating a page containing non-navigation tiles on the same screen is unavailable when it would duplicate those entities. Explain the reason and offer the separately named operation **New empty page with this top bar**; never silently discard tiles from a requested full copy. Copying a bar remains supported and gives each copied item/control a new instance ID. A full-layout replacement on another screen is validated against that destination, rather than appended blindly. Page deletion identifies incoming navigation tiles that will also be removed; undo restores the page, those tiles, Home and workspace position together.

### Content, appearance and interaction

Separate what supplies a value, how it is presented, and what a tap does. Define tagged variants for existing entity sources, built-in content and navigation controls; define only the views and actions currently supported. Validate legal combinations rather than collecting unrelated optional fields in one unrestricted options object.

Reuse entity formatting, icon selection, local-time formatting and existing action validation between surfaces. Keep separate tile and top-bar renderers and separate allowed capabilities: an information item does not automatically acquire all the controls of a full tile. The current state/last-change and always/active choices stay supported. Future attribute bindings, alternative views or top-bar tap actions can extend a specific variant without changing page ownership. Do not implement arbitrary expressions, scripts or custom plugins in this refactor.

The top bar has named leading, title and trailing regions. This gives navigation a home in the model and leaves room for later controls without adding a new top-level flag for each one. It does not introduce arbitrary placement, extra rows or page-link buttons now. New variants must meet firmware capabilities, touch-area limits and explicit overflow rules before the editor offers them.

### Stored configuration and live state

Persist user choices and references. Keep HA values, availability, clock ticks, active page, preview measurements and resolved firmware indexes in separate runtime state keyed by stable IDs and the active revision. Receiving a sensor update must not dirty the editor or rewrite stored layouts. Losing an entity temporarily must not delete its configuration. Reconnects rebuild resolved data from the document and the current HA state.

Copied and exported layouts carry configuration only. Connection keys and device-owned settings remain in their existing stores. Cross-screen import validates against the target grid and capabilities and previews any required reflow or unsupported content. Never flatten several source pages together or drop items silently to fit a different board.

The screen inventory now includes an optional `api_key` for the sidebar's pairing action. It is connection metadata, never part of `ScreenLayout`, page operations, undo history, workspace metadata or layout exports. Build those documents from explicitly allowed fields rather than copying the entire inventory screen object. Preserve the existing profile/key association during migration and screen renames; a screen without a matching profile has no key to copy.

### Capabilities and one resolution pipeline

Derive one capability description from the actual board shape, verified firmware/protocol support, reported runtime features and available setting entities: allowed item/control kinds, grid, page and tile limits, six trailing items today, image support and transport bounds. Centralize existing firmware-version checks in that boundary. Use the existing device information/reporting paths to establish what firmware is actually running; cached Home Assistant registry data is not sufficient evidence to enable a new transport mode after a reconnect. Both the editor and backend use this description, with the backend enforcing it before persistence and sending.

Preserve `features_of`/`able` semantics: an explicit Screen features report wins over the board catalog for supported runtime features, and `none` means no reported features, not an absent report. Keep the existing catalog fallback for older screens or unavailable reports, without treating that fallback as proof of support for the new protocol. A custom profile can enable dimming or standby without changing the board identity or firmware version. Refresh effective capabilities and entity availability after reconnects; do not cache them solely by board/version. Keep Wake, Sleep, standby/night settings and return-on-standby consistent with the actual `CAN_STANDBY` value, including the HA entity visibility fixed in 0.2.131. This does not make grid size or tile capacity editable page settings.

Use a documented pipeline: stored document, validation, resolution against device capabilities and live state, then preview/device output. Include ordered page IDs and instance-to-index mappings in the configuration revision calculation so identical-looking pages cannot alias after reordering. The preview and serializer use the same manager-side value formatting; TypeScript and C++ geometry implementations must pass the same fixtures. This is a shared contract, not a claim that all three languages can execute one implementation.

### Atomic edits and schema evolution

Centralize document operations such as moving a tile, moving a page, copying a bar and deleting a page. Each operation validates references, placement and limits and creates one undoable edit. UI handlers must not independently update related arrays. Define a save as replacement of a validated document at an expected revision; a stale save returns a conflict instead of overwriting newer work. A full event-sourcing framework is unnecessary.

Serialize the read/check/write storage operation across all writers of the shared store, including writes for different screens and workspace metadata. A per-screen revision check alone must not allow two whole-file replacements to discard each other's changes. Validate and durably commit a save before scheduling delivery. If persistence fails, leave the committed document unchanged, retain the browser draft and send none of that attempted save. If delivery fails afterward, keep the successfully saved document and retry delivery; do not roll it back or report that saving failed.

Track the latest saved document revision separately from the configuration/session acknowledged by the device. Report **Saved, waiting for screen**, **Applying**, or **Applied** as appropriate. Only an acknowledgment matching the current delivery target can mark that target applied; a late acknowledgment for an earlier save cannot clear a newer pending update. A lost HTTP save response is resolved by rereading the authoritative saved revision before retrying, without discarding edits made in the browser since that request. These states do not require another Save or activation approval.

Keep storage schema, API/export format, configuration revision and firmware capabilities distinct. Maintain ordered migrations and fixtures for supported historic documents. Unknown future schema versions are preserved and refused, never rewritten with fields removed. Do not create an unvalidated catch-all `extensions` object as a substitute for schema evolution.

### Feature boundary for the first release

| Include in the refactor | Enabled later, with separate product and firmware work |
| --- | --- |
| Page-owned tiles, local positions and read-only source-grid metadata | Portable templates that adapt within each board's fixed grid |
| Stable IDs for pages, tiles and bar items | Multiple entity tiles where current firmware rejects them |
| Typed navigation targets, selectable Home, pagination opt-out and bounded detail Back | User-configured top-bar page links |
| Named top-bar regions and typed content/view/action boundaries, preserving existing tile options | Additional top-bar controls, top-bar attribute bindings or richer conditions |
| Capabilities, atomic edits, complete export and tested migration | Linked presets or reusable room templates |

For each deferred example, verify in review that it could be added through a new variant or capability without replacing the page model. This is a design check, not a commitment to ship those features.

The optional Advanced editor is a separate deliverable on this same foundation. Its workspace, connectors and simulator must not hold up a validated release of page ownership in Simple, and do not justify extra firmware structures. Completing both remains the overall design goal. Keep the typed tile conversion as a lossless boundary around existing card behaviour; do not rewrite working tile renderers, add new control combinations or introduce a generic widget framework in this refactor.

## Editor behaviour

The spatial editor proposal is specified in [Page editor and navigation map](EDITOR_NAVIGATION_DESIGN.md). Stored page order, participation in sequential navigation, navigation actions and workspace positions are independent. The map shows and edits actual supported navigation actions; it does not change board capacity or introduce spatial swipe directions. Workspace positions live outside the device configuration and do not trigger firmware synchronization.

Keep **Simple** as the default editor mode: the familiar horizontal page row, tile library and direct drag-and-drop onto each page. Users can edit per-page top bars, designate Home and reorder pages without entering the map. **Advanced** is an explicit, remembered option for spatial arrangement, connections and route inspection. Both modes share one document, draft and undo history; switching preserves all configuration and map positions and does not send anything to the screen. Simple is a permanent supported workflow, not a legacy data or firmware mode.

Clicking a page's top bar opens **Top bar for page N**, editing only that page. The preview, inspector selection, add-item drawer, drag-and-drop and undo all carry the stable page ID explicitly. Remove implicit global selection from top-bar mutation helpers.

Page settings in either mode include **Do not show in page navigation**, unchecked by default. All pages remain visible and editable in the editor, with a **Deeplink** badge on excluded pages. Keep the full page-order control distinct from a preview of the device's filtered paginator. Changing exclusion is one undoable draft edit, persisted and sent through the normal Save flow; it does not change tiles, top bars, IDs, Home, map positions or current selection. Copying a page preserves its flag; creating a new page starts unchecked. Warn about missing entry or manual return routes using actual enabled navigation, including the built-in Back route without silently adding tiles.

Show one draggable house marker in the page-label strip above the previews, beside the existing page numbers. Dropping it on another page sets `homePageId` without moving pages or tiles. The designated page displays the marker and an accessible Home label; page numbers retain their usual ordinal meaning. Provide a **Set as Home** page action for keyboard and touch users who do not drag. Changing the designation is one undoable draft edit, sent through the existing Save flow, and does not itself navigate the physical screen. Reordering pages carries the Home marker with its page ID.

The editor marker designates the destination. It is separate from the Home control shown on the physical screen: hiding that control does not remove the destination or disable other routes Home. Existing layouts start with the marker on page 1. A new layout assigns its first created page. Capability messaging prevents saving a designation that older firmware cannot represent.

Offer **Copy top bar from...** and **Copy to selected pages...**. These are explicit copy operations with undo, not ongoing links. Let the user choose whether to copy items only or the whole bar, so copying a clock and temperature does not accidentally rename several pages. New pages start with a copy of the selected page's bar, or the standard clock bar when creating the first page; the UI states that starting choice.

There is a deliberate tradeoff: changing one page no longer changes every page. Bulk copy makes that operation available. Linked presets could be a separate future feature if demand justifies them; they are not needed to resolve this request.

Reordering moves the complete page record. Internal navigation references remain unchanged because they identify a page, not its current position. Deleting handles incoming Go to page tiles according to the existing behaviour. Undo restores the whole operation. A copied page and its child instances receive new IDs and independent configuration. Keep numeric external page-navigation commands working as they do now.

`TopbarSvg` should take explicit resolved page input. It should not reach into a global bar as a fallback. All visible page previews need live values, including pages that are not selected. Associate asynchronous preview responses with their request content and context so switching pages or screens cannot apply stale results to another bar.

Update layout copy, export, import, diagnostic layout writers and any API consumers at the same time. In particular, the current `LAYOUT_KEYS` export list and `adopt()` implementation select fields individually; adding a field to the editor type does not make those paths preserve it.

## Manager, device protocol and rendering

Separate four responsibilities:

| Responsibility | Proposed boundary |
| --- | --- |
| Page configuration, validation and page operations | A page-layout model in the manager and editor, with matching fixtures |
| HA state formatting and active-only filtering | Reuse `screen_manager/app/header_bar.py`, accepting one page's items |
| Device delivery and compatibility | `server.py` serializers and per-screen/per-page send caches |
| Geometry and drawing | LVGL-free header model/placement, plus a dedicated LVGL top-bar renderer |

The manager watches the union of entities from every page, not only the visible page. Maintain an entity-to-page dependency index. A state change rebuilds and sends only affected bars; a shared entity updates every page that uses it. Title, item, order and page-count changes affect the configuration revision. Ordinary changing values do not resend the tile layout.

Compile `homePageId` to a validated page index for the active layout revision. Send it as required new-format layout metadata, outside the frozen eleven-field device-settings block, and activate it with the matching page configuration. A Home change affects the configuration revision. The add-on's serializer for old firmware retains that firmware's first-page Home behaviour and refuses an unrepresentable destination, including after a reorder. New firmware has no legacy-manager fallback for a missing Home target.

Compile pagination exclusion into a bounded per-page bitmask in the same new-format metadata. Validate it against the complete page count, include it in the configuration revision and activate it atomically with tiles, Home and every page bar. Scan the existing page records for sequential navigation instead of storing duplicate page layouts or a second configuration. Preserve the active page by ID when its exclusion changes. Firmware capability checks cover exclusion as well as page-owned bars: older firmware cannot silently receive an excluded page as an ordinary sequential page. The legacy serializer accepts only all-false exclusion flags; imports, API writes and downgrades follow the same restriction. Missing required new-format metadata is a configuration error, not a firmware compatibility fallback.

Before replacing configuration, retain only the active page's compact ID. After activation, find it in the new page-key table, or use the new Home if it was deleted. This works even when the manager restarted or two pages look identical, without keeping the previous page document. Dismiss layout-dependent detail overlays and cancel pending touch/slider gestures before replacing their tile records. Stale callbacks must not act on a new tile that happens to reuse an index.

Audit every hard-coded return to page zero and every check for being away from page zero, especially the shared `go_home` script, idle timeout, settings/card return paths, standby and bottom-edge swipe in `packages/core.yaml` and the shared component. Explicit numeric Show page commands keep their original meaning. After a cold boot, the first complete received configuration opens its designated Home page; later configuration updates retain the currently visible page by ID when possible and do not jump merely because the designation changed. If that page was removed, select the new Home. Before configuration is available, keep the existing startup behaviour.

The new editor and on-screen UI describe returning Home. Preserve existing Home Assistant entity IDs, unique IDs, preference keys, setting keys, native setting entity names and discovery markers so existing automations and saved settings remain intact. This does not require accepting an old add-on's layout protocol. Home destination belongs to the layout; the existing auto-return switches and delay remain device-owned settings.

Use an explicit `v: 2` inbox protocol for the new firmware. All inbox operations from a compatible manager, including keepalive and value updates, use that envelope. Preserve reusable message bodies where appropriate, but compile only the new layout/header decoder and renderer into new firmware. The add-on selects `v: 1` serializers for old firmware and `v: 2` for new firmware. Update diagnostic senders as well. The page-header operation identifies the configuration revision, synchronization attempt, page index and compact page ID and carries a complete resolved bar: title, page Home visibility and current rendered items. Exact field names and all mandatory fields should be fixed in protocol fixtures before implementation.

Use bounded begin/commit metadata, one complete header message per page, and bounded tile-initialization records. The current message limit is 4096 bytes; do not append all page keys and bars to the already constrained legacy tile-layout message. Separate records allow a previously valid near-limit layout to migrate without exceeding that bound. Validate every encoded message, including escaping and worst-case UTF-8 text, before replacement begins. Do not raise the message limit to make this fit.

For a changed configuration, use one bounded configuration store, not a complete active copy plus a complete pending copy. A validated begin message declares the revision, synchronization attempt and expected page/tile counts within board limits. Once replacement begins, disable layout interactions, show a lightweight loading state using existing UI elements, and reuse the configuration store for the incoming data. Track completeness with bounded counters/bitmasks and transaction identifiers. Validate each message before incorporating it and reject conflicting revisions or attempts. Activate on commit only after page keys, placement, Home, exclusion, every bar and every tile's initial record have been validated together; no partially replaced page becomes interactive.

This requires changing an existing protocol boundary: current `state` messages also carry tile size, display, tap action and other configuration. A tile list plus page bars is therefore not a complete layout. New tile-initialization records must include all required tile configuration and an initial state snapshot, including an explicit unavailable state when HA has no value. Keep one tile object per instance. Subsequent value updates cannot replace placement, navigation targets or saved tap configuration. Derived values/control availability still follow HA capabilities, but must not restore options from an older configuration. Do not wait for optional history, forecasts or images to finish before activation; these may populate afterward through their existing bounded request paths.

Interrupted or invalid replacement keeps the configuration inactive, shows the loading/retry status and requests a fresh synchronization from the add-on's committed document. The previous layout is not retained as a second configuration for rollback. A timeout remains retryable and the existing OTA, connection and settings/recovery paths remain available. This intentionally trades a loading state during structural changes for a lower peak RAM requirement; measure its duration on a maximum supported layout rather than promising it is imperceptible. An unchanged revision must not start replacement when that complete configuration is already active. After a reboot or incomplete replacement, the same saved revision must be eligible for a full recovery synchronization with a fresh attempt; revision equality alone does not prove readiness. An ordinary keepalive never clears the incomplete state or restarts an in-progress transfer. Subsequent bar-value updates validate at most one bounded page record before replacing it, without rebuilding or duplicating the whole configuration. Duplicate messages are harmless and obsolete revisions never overwrite active data.

Include tile options and header configuration in revision calculation and synchronization checks. A tile-list acknowledgment alone must not mark a configuration synchronized when initial tile records or page bars are missing. Live tile/header values are excluded from the configuration revision. After activation, ordinary tile-state delivery remains separate from configuration replacement.

Use one serialized sender per screen. Bind every indexed message to its revision and synchronization session, and keep bounded sequence numbers for changing tile/bar values so delayed packets cannot overwrite newer values within the same revision. Scope history/image/options replies to the requesting configuration and view as appropriate. Define duplicate begin/commit handling, superseded attempts, reconnect and manager-restart handshakes in protocol fixtures; a late begin must not restart an already superseded transaction. Coalesce updates arriving during synchronization in the manager and send the latest values after activation, without an on-device backlog. Retry transient failures with bounded backoff; report a deterministic invalid document or insufficient-memory refusal instead of entering a perpetual reload loop.

Firmware holds bounded data for each configured page and uses one reusable set of LVGL widgets. Page navigation selects the cached bar and lays out title, Home and items in the same render pass as the new tiles. There is no HA round trip on a swipe, Home tap, Go to page tile, timeout or automation-triggered page change.

### Firmware memory contract

Compatibility belongs in the add-on. New firmware contains one page-aware runtime and one supported inbox protocol. Remove the old global-header receiver, separate legacy page-title representation, old render fallbacks and legacy conversion paths when replacing them. Do not keep two configuration models, an old/new mode switch, duplicated LVGL trees or compatibility-only caches on a board. Simple/Advanced mode, workspace positions, entity-library data, undo history and migration backups remain off-device.

Reuse the existing bounded JSON transport and inspect the envelope version before decoding configuration into the runtime model. A known old `v: 1` message is rejected with **Configuration problem. Update add-on.**, displayed through the existing status/error surface and returned through the normal status path. The rejection needs only a small status value and fixed message text; it must not decode or convert legacy configuration, mutate preferences, accumulate rejected packets or repeatedly recreate UI objects. A missing connection is a waiting/offline condition, not evidence of an outdated add-on. Unknown future versions or malformed new-format data get an accurate generic protocol/configuration error rather than a misleading instruction to update the add-on.

The version guard, message text and normal parser scratch space are not literally zero bytes. The requirement is no legacy implementation or duplicate configuration retained for the transition. Independent per-page content also has an inherent storage cost compared with a single global bar. Measure and report that separately from compatibility, including peak RAM during synchronization, live page updates and rejection of repeated old messages. Keep configured data compact, allocate within actual page needs and board limits, and reuse the displayed widgets. No claim of unchanged total firmware size or RAM is valid without measurements.

Extract the rendering code from `runtime_tiles.h` behind explicit inputs and navigation callbacks. Keep full title text in the model; never measure a label after LVGL has shortened it. Reuse `header_bar::gaps/place` and keep preview and firmware geometry aligned through shared test fixtures. Geometry continues to use the shared UI sizing helpers and board substitutions, and colours continue to use theme roles.

When the HA feed is unavailable, preserve current semantics: local clock/date items keep working, HA-derived values are hidden, and local navigation remains available. Detail cards, alerts and settings retain their own headers and layering. The Home touch target must continue to respect the touch guard and the settings long-press region.

## Storage migration and compatibility

Use storage version 2 for the structural change. Keeping the new records under version 1 would allow an older add-on to load and rewrite them through a validator that does not preserve the new model. Storage schema and inbox protocol versions are independent contracts even though both become version 2 in this proposal. Device preference structures and the existing eleven-key settings contract remain unchanged.

Storage migration and firmware rollout are independent. Migrating a layout in the add-on neither flashes its screen nor requires every screen to be online or upgraded. Keep compatibility per screen in the add-on, with no one-week deadline or installation-wide switch to the new transport. Update the add-on first for uninterrupted operation; firmware-first is a detectable mismatch that shows an update message until the add-on catches up. This explicitly replaces the earlier proposal for a legacy input adapter in new firmware.

Implement and test migration before enabling the new editor:

1. Read and validate the version-1 envelope without modifying it and make a durable backup of the original file in persistent add-on data. Never overwrite that pre-migration backup on later starts, and include it in add-on backups. Wrap records in a version-2 envelope with an explicit per-screen layout format. A screen's entry holds either an untouched legacy payload awaiting conversion or a converted page document, never two competing editable copies. Unknown outer versions or invalid JSON leave the original file untouched and report the problem.
2. Convert each screen independently. Determine its layout's grid from its saved profile or verified screen shape, preserving intentionally empty pages as well as tile-derived pages. Read the original records before the current loader normalizes them: `validate_layout(..., grid=None)` currently packs tiles without slots using the default grid. A legacy record without positions must be packed using its verified source grid, never an assumed CYD grid. A temporarily offline screen with reliable saved board information can be converted immediately. If information is missing or an entry cannot be converted losslessly, retain that legacy entry and its existing compatible path, show a per-screen migration status and retry when the missing information is established. This must not block conversion, saving or delivery for the other screens.
3. Materialize page records and assign stable IDs to pages, tile instances and header instances. Split tiles by their original page and convert absolute slots to local row/column positions using the verified board grid. Persist the source-grid snapshot as manager-owned metadata and set `homePageId` to the first page's ID. Set `navigation.excludeFromPagination` to `false` on every page, preserving the existing sequential navigation. Convert numeric navigation entities to typed page-ID references. Copy the legacy global header independently into each page's trailing items. If absent, use the old `show_clock` rule. Convert each active page title to its explicit title source. Retain titles beyond the active page count as migration metadata for recovery; those entries must not create additional visible pages.
4. Preserve gaps, tile options, page order, settings and screen keys exactly through the legacy serializer. Map existing options into typed placement, content, appearance and interaction fields without dropping supported values. Any stored field that cannot be represented losslessly leaves that screen pending conversion rather than disappearing. Add a leading Home control to each page without changing the existing device setting. Prove migration by comparing the legacy serialized output and rendered behaviour before and after conversion, excluding intentionally new revision metadata.
5. Validate the complete replacement envelope, including the unchanged pending legacy entries, and write it atomically using a temporary file, file flush/fsync, replacement and the required directory durability step. Never install the replacement if backup or write fails. A restart sees a complete old or new document. Converted IDs and per-screen format markers are committed together and remain stable on every restart. Every subsequent writer, including setting updates, screen renames and legacy API edits, must preserve pending records and use this common storage boundary.
6. Restore the appropriate backup when rolling back to an add-on that cannot read version 2. Preserve the current version-2 store separately before restoring anything. Unknown future versions remain an explicit refusal, never an empty layout.

Do not invent pages or silently retarget a legacy navigation tile whose numeric target lies outside the effective page count. The old runtime may have clamped such a target; a stable-ID conversion must expose that ambiguity and leave the screen pending until it is resolved. Compare legacy header representability by content/options after removing new instance IDs, otherwise the independently identified copies created by migration would incorrectly appear incompatible.

Compatibility matrix:

| Combination | Required behaviour |
| --- | --- |
| New manager, new firmware | Full page-specific bars |
| New manager, old firmware | Project representable layouts to the existing header/title messages; allow existing operation, require a firmware update before saving independent items, a non-first Home page, pagination exclusions or other unsupported page behaviour |
| Old manager, new firmware | Reject the old protocol and show **Configuration problem. Update add-on.** No legacy rendering path |
| Older browser/API client, new manager | Preserve page records for unambiguous edits; reject ambiguous structural/global-bar writes with a reload/upgrade response once page-specific data exists |
| Old add-on, version-2 store | Refuse the unsupported version; documented backup restore for rollback |

Give the editor API an explicit format/capability marker and optimistic revision checks. An old browser must not silently flatten bars or associate them with the wrong pages after reordering. Update exported-layout format separately and accept old exports through the tested converter. If newer configuration is already saved and firmware is downgraded, preserve the data and report the mismatch instead of silently selecting one page's bar for all pages.

### Updating the add-on first

The add-on can migrate storage immediately while every screen still runs its old firmware. For each old screen, the legacy serializer produces the same supported layout, global header and page-title messages it received before. It never sends the new page-header operation to firmware that cannot parse it.

Existing supported editing remains possible. In the editor, changing shared header items for an old screen is an explicit all-pages edit that updates every page's independent item array together. Per-page titles continue where that firmware already supports them. Different page item sets, page-specific Home visibility, a Home target outside the first position or excluded sequential pages require a firmware update for that particular screen. The backend checks the same rules, including imports and API writes; disabling controls in the browser alone is insufficient. Reordering that would move Home away from the first position must also be checked before saving.

The add-on retains the full new document internally. The old wire representation is derived for sending, not another editable saved layout. It never chooses one page's header and silently applies it globally when the page configurations differ.

For a screen known to require newer firmware, show **Update screen to use the new titlebar and layout** in its editor, with an **Update screen** action leading to the existing firmware-update flow for that screen. Keep supported editing available. This notice is specific to the selected screen, not an installation-wide restriction. Firmware installation remains a user action; switching to the new layout once supported is automatic.

Before that flow installs new-protocol firmware, verify that this screen's saved layout can be converted and serialized completely. If migration is pending, show the specific missing grid information or invalid reference to resolve first. A firmware update performed outside the add-on cannot enforce this preflight; retain the data and show the actual migration problem rather than falsely claiming that matching software versions guarantee a usable configuration.

### Updating one screen later

On reconnect, establish the running firmware from fresh device information or a fresh report associated with the connection. A successful build/upload job, the latest published version or a stale cached registry entry is not proof of what the screen runs. Until support is established, do not send new operations or discard the stored layout. Keep the last compatible delivery behaviour where it is safe to do so, otherwise show that capability confirmation is pending.

For a screen confirmed to support the new format, invalidate only its transport/send cache and perform a full configuration sync using the single-store replacement flow. Receive all required configuration into that bounded store, then activate the page headers, layout and Home target together. Keep the screen marked as syncing until it confirms the active configuration, including the required page bars; successful calls to Home Assistant are not acknowledgment of device activation. Interruptions retry from the saved configuration. Storage does not migrate again when firmware changes.

Start this transition immediately when the updated add-on has confirmed the screen's support and its stored layout has converted successfully. There is no opt-in, separate migration button, manual Save, browser refresh or add-on restart required. The already-open new editor observes per-screen capability and delivery updates: replace the update notice with **Applying new titlebar and layout**, then remove it after acknowledgment of the complete active configuration and enable the supported page controls. If the device is offline or its running version is unknown, show **Waiting for screen** or **Checking screen compatibility** instead of claiming that activation succeeded. Retry interrupted synchronization automatically.

Preserve the chosen Simple/Advanced editor mode throughout this automatic transition. New per-page functionality becomes available in both modes; activating the new format must never force the navigation map on a user who prefers Simple.

Automatic activation sends the latest committed layout and preserves its existing appearance. Preserve any unsaved browser draft, selection and workspace positions; do not discard or implicitly save that draft when capabilities change. Draft edits continue through the normal Save flow and revision checks. If conversion is pending because required information is missing, expose that specific status and resume automatically once resolved, rather than asking for another activation approval.

The other screens continue using their own serializers, caches and capability restrictions. Updating one screen never enables unsupported controls or sends new operations to another screen. Keeping this mixed installation for a week or longer is a supported state.

### Updating firmware first

New firmware accepts only the new inbox protocol. When an old add-on sends its `v: 1` configuration or keepalive, the screen shows **Configuration problem. Update add-on.** It does not translate or render that old configuration. Normal layout use is unavailable during this mismatch; connectivity, OTA and recovery remain available. Repeated old packets keep the same status without increasing retained memory or causing UI churn. No arrival-order heuristic, fallback global bar or legacy decoder is present.

When the add-on is updated later, it migrates its own stored data, confirms the running firmware and automatically sends the complete new-format configuration. The screen transitions from the update message through loading to normal operation after validating that configuration. A successful flash, version report, isolated page header or keepalive alone cannot clear the mismatch and claim a working layout. No additional approval, reboot or user Save is required. Preserve the user's layout choices and Simple/Advanced editor preference.

The old add-on cannot be retroactively taught the new editor notice, so the on-device message is the reliable signal for this order of updates. Recommend updating the add-on first in release instructions. Continue checking the firmware against the project's supported ESPHome build versions; that is separate from compatibility with old add-on messages.

### Example of a gradual rollout

| Time | Add-on | Screen A | Screen B | Behaviour |
| --- | --- | --- | --- | --- |
| Before updating | Old | Old | Old | Existing behaviour |
| Day 1 | New | Old | Old | New stored model where migration can complete; legacy messages to both screens |
| Day 8 | New | New | Old | Page-specific features available on A; B retains its supported editing and legacy messages |
| Day 15 | New | New | New | Both screens can use page-specific bars and a selected Home |
| Alternative day 1 | Old | New | Old | A shows **Configuration problem. Update add-on.** B operates as before |
| After updating that add-on | New | New | Old | A automatically receives and activates the new configuration; B continues receiving legacy messages |

Elapsed time has no effect on format selection. A screen reappearing after weeks is checked and resynchronized individually. Boards retain their own fixed grids throughout every stage.

### Rollback and interrupted updates

The add-on-first path preserves operation throughout a gradual rollout. Firmware-first intentionally waits with the update message until the add-on is compatible. A deliberate firmware downgrade after saving new-only features is a different case: old firmware cannot express independent bars or a non-first Home target. Retain the new configuration and explain that the screen needs compatible firmware, or offer an explicit restore of a known compatible configuration. Do not silently flatten or overwrite it. After a reboot, unsupported old firmware cannot be promised to show that new configuration.

An old add-on cannot read the version-2 store and must refuse it safely. Rolling it back requires restoring its version-1 backup while retaining a copy of the latest version-2 data. A pre-migration backup does not contain edits made afterward; state that clearly in the rollback instructions. This is not a lossless automatic downgrade, and restoring it must not happen without an explicit recovery action.

Restoring that backup alone does not make new firmware accept an old add-on. Those screens show the same update-add-on message until a compatible add-on is restored, or a separate deliberate firmware downgrade and compatible-layout recovery is completed.

A failed firmware upload does not change stored data or capability status based on the job alone. After an interrupted add-on migration, a power failure during storage replacement, a lost header packet or a disconnect during synchronization, restart from the last committed data and establish what each device actually received before marking it synchronized.

## Implementation sequence

1. **Characterize and prove the risky boundaries.** Record current render and protocol behaviour and measure the existing CYD flash and heap baseline. Before a storage cutover or large editor build, prove migration equivalence on old records and a minimal new-runtime prototype with complete tile initialization, compact page keys, all page bars and interrupted replacement. Fix message fixtures and memory measurements first. Extract the renderer without changing visible behaviour.
2. **Introduce the document model.** Add page-owned tiles, local positions, manager-owned source-grid metadata, stable instance IDs, typed navigation, `homePageId`, the default-false pagination opt-out, top-bar regions and centralized edit operations. Enforce the board's fixed grid and capacity. Define the content/appearance/interaction variants for existing features and consolidate capabilities. Add versioned API/export contracts, legacy serializers and migration-equivalence fixtures. Do not enable the storage cutover until both consumer paths are ready.
3. **Add device delivery.** Implement the new-only inbox protocol, a minimal version-rejection status, page-header parsing, the bounded exclusion mask and filtered sequential navigation, bounded single-store replacement, completeness/revision checks and dirty-page delivery. Remove superseded layout/header decoding and rendering paths. Measure worst-case flash and peak RAM on the CYD before committing to the layout of those records. Keep old-firmware serializers in the add-on only.
4. **Connect Simple, then Advanced.** Make every mutation and preview page-specific; add the draggable Home marker and accessible Set as Home action, the opt-out checkbox and Deeplink badge, filtered paginator previews, capability-aware copy operations, undo, capability messaging and complete import/export support. Complete the familiar Simple workflow first. Add workspace, derived connectors, focused editing, navigation preview and reachability through the companion UX stages, without another firmware model. Build and commit the generated editor assets with the sources when implementation is released.
5. **Integrate and release.** Exercise migration and mixed-version paths, complete render and hardware acceptance, update user documentation and release notes, then publish under the repository's release process.

These are reviewable implementation stages, not a requirement to publish five partial releases. Every pushed release still needs the add-on version and CHANGELOG update required by `docs/RELEASING.md`.

## Acceptance criteria

- A migrated layout renders as before, including an absent header, an intentionally empty header, per-page titles, Home disabled through HA, gaps, full-page tiles and intentionally empty pages.
- Pages show different entity sets correctly. The same entity can appear on several pages with different formatting or active-only rules. Updates reach hidden pages without waiting for navigation.
- Add, reorder, remove, undo, copy, save, restart, import and export preserve each page's entire bar and navigation targets. Cross-board copies explicitly handle pages that cannot fit rather than discarding configuration silently.
- Page and instance IDs survive edits, save/restart and undo; copies receive new IDs with correctly remapped references. Reordering visually identical pages still changes the configuration mapping. Entity state updates do not alter saved configuration, and stale document saves cannot overwrite newer edits.
- A prohibited same-screen page copy explains the duplicate-tile restriction and leaves the draft unchanged. Creating an empty page with a copied top bar is a distinct operation. Existing tile attribute subtitles, custom actions and every supported size/control combination survive lossless conversion.
- The Home marker moves independently of page order and tiles, follows its page when reordered, survives save/restart/export/import, and has equivalent drag, keyboard and touch interactions. Deleting and undoing the Home page, copying it and importing a full layout maintain exactly one valid Home target.
- All Home routes reach a non-first designated page, including the button, gesture, idle timeout, enabled standby return and Go home action. Idle detection uses the selected Home. Numeric Show page 1 still opens the first page. Changing Home does not immediately navigate the device; the first complete configuration after cold boot selects Home.
- Preserve the 0.2.131 standby override: test the stock capability report and an override-enabled report on the same board/version, plus explicit `none` and unavailable reports. HA entity visibility and editor settings agree with effective support; an enabled return-on-standby reaches the selected Home. Capability refresh does not rewrite page data or reset device preferences.
- The pagination opt-out is unchecked for new and migrated pages. Five configured pages with two exclusions form a three-page sequential paginator while all five retain their tiles, bars, IDs, links and numeric Show page destinations. Included-page order is derived from the complete collection, not saved as a competing order.
- Excluded pages remain editable and reachable by explicit navigation, with no sequential counter, previous/next controls or sequential swipes while on them. Verify explicit return routes, separately gated Back, excluded Home, zero or one included page, nested deeplinks, active-page exclusion and settings that suppress navigation. Reordering, save/restart, copying, import/export, undo and editor-mode switches preserve the opt-out and all board limits.
- Exclusion metadata participates in the same revision and complete-configuration acknowledgment as Home, tiles and bars, without a second page store. Reject out-of-range mask bits, missing mandatory metadata and unsupported old-firmware saves or downgrades instead of silently reincluding pages. Interrupted synchronization must not mix old navigation membership with new page data.
- Board-defined columns, rows, cell count, tile limit and page limit cannot be overridden by page data, imported metadata or API writes. Imports either fit or explicitly adapt within the destination board's existing capacity, or are refused with the source data intact.
- Changing a physical screen's grid cannot silently reinterpret positions or merge pages. Test orientation rebuilds, cross-board copies, a legacy layout without explicit slots and the nine-cell grid's seven-page limit. Preserve the committed source grid until a valid adaptation is saved.
- The nested model flattens to the same tile order, positions, options and navigation behaviour as supported legacy layouts. Unknown schemas, unsupported variants, ambiguous legacy edits and layouts exceeding device capabilities fail without losing data.
- Every navigation route changes tiles, title and bar together. Long/short titles, Home visibility changes and repeated page turns cannot reproduce issue 27.
- Preview and device agree on all item types, languages, number/time formats, active-only items, overflow, light/dark themes and supported orientations.
- Missing, duplicate, delayed, oversized and invalid messages; saves during transmission; HA disconnects; manager restarts; and firmware restarts recover without mixing bars between pages.
- Test that the released old add-on causes the specific update-add-on message on new firmware without legacy rendering or allocation growth, and that upgrading the add-on later automatically restores operation. Test the new add-on against each supported old-firmware protocol tier and old/new firmware screens operating together. Exercise add-on-first, firmware-first, skipped intermediate releases, delayed updates, offline screens and restarts between each step. Use actual previous-version artifacts for integration checks as well as message fixtures.
- A pending legacy record with missing board information survives every other screen's saves, setting changes, renames and restarts byte-for-byte in its payload. It cannot block other screens. Inject backup, write and replacement failures and confirm that the original data or complete committed replacement remains recoverable, with no regenerated committed IDs.
- Cached firmware metadata cannot enable an incompatible transport after a reconnect. Test a failed flash, stale HA registry data and an actual firmware downgrade. Verify new operations are never sent to a screen without established support, and unsupported new layouts remain stored rather than being flattened.
- Keep the new editor open throughout a screen firmware update. Verify that the update notice changes to applying and then disappears after complete configuration acknowledgment, with no refresh, manual Save or activation confirmation. Repeat with unsaved edits and verify the draft survives without being sent, and with a disconnect during synchronization to verify automatic recovery. Other screens retain their own notices and modes.
- Preserve the 0.2.132 sidebar and clipboard behaviour in both editor modes: a paired screen with a matching profile can still copy its API key, and a screen without one offers no invented key. Retain copying over HTTP through the shared fallback and manual selection when copying fails. Use synthetic keys in tests and verify that page copy, undo and layout/workspace export never include the inventory's `api_key`.
- Run the mixed-version cases across simulated week-long delays and resync intervals, and retain a hardware mixed-version soak with its actual duration reported. Verify an older open browser cannot overwrite page-specific data or move Home through an ambiguous save. Check rollback restores the selected backup and clearly accounts for later edits.
- Exercise each board's maximum supported pages with six top-bar items each, maximum tile occupancy and longest permitted strings. Measure flash, peak RAM, minimum free heap, largest allocatable block during sustained updates, page-switch latency and complete synchronization duration, including JSON parsing, one-record scratch data and configuration replacement. Include camera/history/detail-card workloads, not only an idle top bar. Separate new-feature cost from the small version guard. Verify there is no legacy decoder, global-bar fallback, duplicated widget tree or second complete configuration store. Apply the existing release flash budget; a feature cannot ship in a fixes-only budget tier. Set a measured RAM headroom gate from the current working baseline before approving the implementation.
- Interrupt single-store replacement after every message boundary and verify that incomplete data never becomes interactive, the device requests a retry, the add-on resends its saved document automatically and OTA/recovery remain available. Repeated keepalives and value-only changes must not flash the loading state or rebuild the whole layout.
- Recover an interrupted transfer and a device reboot by resending the same saved configuration revision, without another user edit or Save. An already active complete revision still avoids replacement on routine keepalives.
- Save from two browsers and to two screens concurrently, including a workspace write, and verify that no committed data is lost. Inject storage failure, delivery failure and a lost HTTP save response. Preserve the appropriate draft/saved state, send only committed data and recover automatically after a delivery failure. A delayed acknowledgment for save A cannot mark newer save B applied.
- Delay the last tile-initialization record, especially a wide/full tile or custom tap action, and verify activation waits. Delay old state packets and indexed callbacks across reorder, save, reconnect and manager restart; they cannot restore old geometry, trigger another tile or regress newer values. Keep the viewed page by compact ID, including reorder of identical pages, or choose Home when that ID was deleted.
- Run `tools/check.sh` and `tools/check.sh --firmware` across every board in `tools/profiles.py`. Use both supported ESPHome build versions where required by the release instructions. Extend `tools/render_topbar.py` beyond its current CYD/Guition scenarios to cover page transitions and representative widths/orientations of the shipping boards.
- Obtain physical tap, swipe, Home and settings-long-press acceptance on available CYD and Guition hardware, plus available larger screens. Record duration and which boards were actually exercised. Builds and rendered screenshots do not substitute for physical acceptance.

The first release should finish page ownership and add-on-side compatibility while preserving the current device visual design. New firmware deliberately rejects old add-on messages. Arbitrary top-bar page-link buttons, tappable top-bar entity actions, linked presets, extra top-bar rows and changes to the six-item top-bar limit remain separate product decisions. Existing navigation tiles and tile actions remain supported.

### Migration lifetime and cost

Run storage conversion once per screen, when its original grid is known. Persist the completed v2 document and its stable IDs. Normal saves, previews and updates do not rerun a migration. Check pending records only when relevant device/profile information changes. Compile a derived delivery view once per saved document revision; changing HA values reuses that view and never converts persistent documents back to v1.

Keep historic storage readers isolated from the current domain model. They exist only in the add-on and execute on opening an older store or explicitly importing an older export. Removing a reader later requires a documented minimum supported source version and an intermediate upgrade or offline conversion route for users who skipped releases. Remove a legacy wire adapter independently when support for its firmware tier ends. Neither migration nor a legacy protocol decoder ships in v2 firmware.

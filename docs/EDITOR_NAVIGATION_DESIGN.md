# Page editor and navigation map

Status: implemented in the 0.3.0 editor. This document records the UX contract and acceptance requirements. See [Pages](PAGES.md) for the user guide and [the page/top-bar refactor](TOP_BAR_REFACTOR.md) for architecture decisions. Earlier interactive concepts used sample data; the production editor reads entities and available history from Home Assistant.

## Recommendation

Keep one editor with two selectable modes: **Simple**, the default familiar editor for placing tiles directly on ordered pages, and **Advanced**, the spatial navigation map. Keep the existing device sidebar, header and board-specific previews. Both modes edit the same document through the same operations and retain the same firmware capability checks.

Simple keeps the existing horizontal page row, visible tile library and direct drag-and-drop into the real page grid. A user with two pages can drag tiles onto either page and save immediately, without opening a map, arranging nodes or entering a separate page-editing step. Keep page addition, page reordering, tile editing, per-page top-bar editing and Home designation available here. A navigation tile uses the ordinary destination picker. Hide graph connectors, map arrangement, zoom and route-analysis panels.

Connection rendering and its pointer handlers belong exclusively to Advanced. Leaving Advanced cancels any unfinished connection and removes its lines and hit areas, including after a drag or selection change. Navigation tiles keep their ordinary destination icon in Simple; graph lines must never remain over the page row.

The entity library has a visible **Add entities** entry and search by friendly name or entity ID. Drag an entity onto a specific empty cell, click an empty cell and choose an entity, or use the add action for the first free cell on the explicitly selected page. Keep the library open after adding so repeated placement stays quick. Mark entities already in use when firmware disallows duplicates. Occupied cells and full pages cannot be overwritten through an add operation. Adding and removing tiles use the shared undo history and preserve empty cell positions. The interactive concept uses clearly labelled sample entities; the production picker uses the existing Home Assistant entity source.

Keep the editor chrome quiet while tiles retain the screen's visual identity. Use Material Design Icons (MDI) consistently in the editor, entity library and tile previews, following the existing screen icon mappings. Use the existing screen theme roles and the entity's supported state, icon and color information for active lights, heating and other controls. Show meaningful values, dimmer tracks, switch positions and sensor history where the corresponding tile supports them. Color must accompany a label or recognizable control, and unavailable or inactive entities must remain distinguishable. A missing value or history series must never turn into invented production data.

Use shared tile presentation rules for the entity library, placed tiles and focused preview, so users recognize what they are adding. Preserve the destination board's cell count, proportions and supported controls. At small map sizes, simplify optional details while keeping readable names and values; focused editing shows the full tile. Selecting or dragging a tile never operates a real device. The design concept may offer explicitly labelled local example controls, whose values stay separate from layout edits and never mark the configuration as changed. This editor work adds no firmware compatibility renderer or duplicate on-device configuration.

Derive controls from both the entity's advertised capabilities and the existing screen card's supported operations. Light color modes determine whether dimming, color temperature or color selection is meaningful; absent brightness on an off light is not a measured brightness of zero. Cover position and tilt are separate values. For climate entities, distinguish measured temperature, target temperature, selected HVAC mode and current HVAC action: heat mode with an idle action must not imply active heating. Use reported limits and steps where available. Resolve MDI icons through the existing Home Assistant icon mapping and state-aware fallback, rather than assuming every entity supplies an explicit icon or color attribute.

Match control combinations to the firmware renderer, not just Home Assistant capabilities. A compact light tile can show the inline brightness slider; its configured tap action handles on/off, without an additional switch. The brightness command clamps to a nonzero minimum of about one percent, so dragging to the minimum must not simulate turning the light off. Direct toggle and setpoint panels require a larger card with enough room. Inline slider and direct control modes are alternatives, not controls to stack together. Keep the existing board grid and tile footprints when representing these choices.

Sensor previews use real units and recorder history when available, with actual timestamps and gaps preserved. Downsample a dense series for the preview without inventing points or hiding significant changes. Clearly distinguish unavailable, unknown and missing-history states from numeric zero. Show the snapshot time when presenting captured real data for design review; keep local access credentials and private entity snapshots outside public documentation and version control.

Advanced adds spatial placement, connections, route inspection and local navigation preview. It can open a focused page editor at a useful size rather than making small map thumbnails carry every editing task. The advanced interface is an explicit preference, not a required step for using the new page-owned top bars or layout.

The navigation view is a graph of real button destinations. Place Home at the top and linked pages below as the initial arrangement, but permit shared destinations, cross-links, cycles and manual placement. It must not imply that every page has a parent or that placement changes navigation.

The scope is pages within one selected physical display. The device sidebar still selects displays. Connecting different physical displays would require separate behaviour and is not part of this proposal.

## Four independent concepts

| Concept | User-visible meaning | Effect on the device |
| --- | --- | --- |
| Page order | The complete ordered collection, including deeplink pages; numeric Show page commands use these ordinals | Reordering changes those ordinals and the relative order of included pages |
| Pagination participation | Whether a page belongs to sequential page navigation | Excluded pages are skipped by the paginator, previous/next controls and sequential swipes |
| Navigation action | A particular button opens a page identified by its stable ID, or goes Home | Creating or changing the action changes where that button goes |
| Map position | Where a page preview sits in the editor's workspace | None; it never changes page order, tile positions or device capacity |

A page at the top of the map can still be page 4. Show its name prominently, its page number secondarily, and the Home badge independently. Never renumber pages just because their map positions changed. The destination board owns every preview's aspect ratio, grid, tile footprints and capacity.

Use the actual board limit in page controls and examples: up to eight pages overall, but seven on the current nine-cell grid because the existing total-cell ceiling is 64. Changing orientation through a rebuild or importing from another grid requires the explicit adaptation described in the main plan. Map placement cannot perform that adaptation or alter the grid.

The editor's page number identifies its position in the complete collection. The device's paginator shows position within the included sequence. Excluding a page changes the latter only, so existing explicit links and numeric Show page commands still address the same pages.

Both existing and new installations initially open Simple, preserving the familiar workflow and page order. Remember an explicit Simple/Advanced choice locally per physical screen. Never select Advanced automatically because firmware was updated, more pages were added or navigation links exist. On first opening Advanced, generate a useful arrangement from actual links and Home. Do not rearrange an established workspace when a sensor changes, a link is edited or Home is moved.

Switching modes changes presentation only. Preserve the complete draft, page and tile selection, undo history, independent top bars, navigation targets, Home designation and saved map positions. Cancel an unfinished connection gesture without changing its source action. Returning to Simple must not flatten configuration, reset links or require migration. The preference never changes the device configuration revision or causes a send. This is a lasting choice of editing workflow, not a switch back to legacy firmware or storage.

## Application structure

The device sidebar remains the place to select physical displays. The screen header keeps Layout, Screen settings, delivery status and Save & send. Within Layout, provide a clearly labelled **Simple / Advanced** editor-mode selector. Avoid an additional required mode chooser on startup.

Preserve the 0.2.132 **Copy API key** action in screen details, including after pairing, in both modes. It remains a device-level action shown only when a matching profile supplies a key. Reuse the existing clipboard helper so HTTP installations retain the fallback copy and manual-selection path; do not assume the secure-context Clipboard API exists or announce success after a failed copy. Inventory credentials stay out of page drafts, undo, workspace data and layout exports.

Screen settings follow the effective features reported by the running screen and available HA entities, including the 0.2.131 standby override. Two screens of the same board/version can expose different standby or dimming controls. Refresh those controls after an update without resetting the page draft or editor mode. Show or hide return-on-standby with actual standby support; it still resolves to the chosen Home and does not count as a manual return route in map analysis.

In Simple, users see the familiar ordered pages and can reorder them through the page headers or keyboard controls. In Advanced, those headers move pages on the workspace only; a compact, explicitly labelled page-order control changes the complete page order, from which the included sequential pages are derived. The distinction must be explicit in the move affordance. Do not repurpose dragging a tile to mean connecting or moving a page. Existing external numeric Show page commands follow the resulting full order; explain that consequence when reordering. Firmware capability checks can restrict an operation, including moving Home away from the first position on old firmware.

The right-hand panel is contextual, with the tile library remaining readily available for repeated placement in Simple. A selected page exposes its title, Home designation and pagination opt-out; Advanced additionally shows incoming/outgoing routes and **Edit page**. A selected navigation tile exposes its label and destination in either mode. Selecting a connection in Advanced opens that same tile inspector, never a separate competing link configuration.

At map scale, previews help recognition and show selected navigation sources. **Edit page** opens the selected page at a useful size, retaining the same inspector and existing board-specific preview. Returning restores the workspace position and selection. The focused editor retains ordinary tile placement, resizing within existing footprints and top-bar editing.

## Pages excluded from sequential navigation

Add **Do not show in page navigation** to page settings in both editor modes. It is an opt-out checkbox, unchecked by default, backed by `page.navigation.excludeFromPagination`. New and migrated pages start with `false`. Checking it excludes the page from the device's paginator, previous/next controls and sequential swipes. It does not delete the page or change tiles, top bars, IDs, explicit links, numeric Show page destinations, Home designation or device capacity.

Keep every page visible and editable in both modes and in destination pickers. Mark excluded pages **Deeplink**; keep their positions in the complete editor page order. Moving one in the map or changing editor mode must never change its opt-out. Unchecking restores participation at the page's existing position. Saving, undo, copying a page, importing and exporting preserve the flag; an ordinary new page starts unchecked even when created from an excluded page's top bar.

A full page copy is subject to the existing restriction on repeated non-navigation entity tiles. If that copy cannot be represented, disable it with an explanation and offer **New empty page with this top bar** as a distinct action. The latter copies only the bar, creates new instance IDs and starts with the pagination checkbox unchecked. Never show a successful full copy after silently omitting conflicting tiles.

For example, Home, Lighting and Climate can form the normal sequence while Scenes and Energy details are excluded. The physical paginator then shows 1/3, 2/3 and 3/3, while all five pages remain configured and count toward the board's existing limits. On an excluded page, omit the counter and sequential previous/next controls and ignore sequential swipe gestures. Explicit links and enabled Home routes continue working. Home can itself be excluded without losing its designation. With zero or one included page, hide sequential controls rather than forcing any page back into the sequence.

Keep the current stop-at-the-ends navigation rather than introducing wrapping. With Page buttons enabled, every page of a multi-page layout reserves the same footer height. Excluded pages replace the sequential controls with **Back**, without a counter or dots. Entering or leaving a detail page never changes tile height. With Page buttons disabled, every page uses the taller tile area and a detail page replaces the leading Home icon with a left chevron for Back. The editor preview and simulator use these same visibility and geometry rules.

Back is built-in navigation and never consumes a tile. It returns along the actual route, including nested detail pages. With no surviving previous destination it goes to the designated Home. It remains available on detail pages even if the ordinary Home control was disabled. Reachability analysis ignores sequential edges to or from excluded pages and includes the guaranteed Back fallback. Never add a return tile or undo the user's opt-out to repair a route.

Changing the checkbox is one undoable draft edit, sent through the normal Save flow. Keep the currently viewed page and its selection even when it becomes excluded. Gate this device behaviour on verified firmware support in the editor and backend; old firmware must never silently ignore the flag. Migration and automatic activation leave all existing pages included until a user explicitly opts out.

## Concrete navigation workflow

1. Select a navigation tile on a page preview. Its destination is highlighted and its outgoing connector becomes available. Other tile types keep their existing meaning.
2. Drag the connector to another page, or choose the destination from a named-page picker. The target page highlights before release and the inspector displays the prospective destination.
3. On a valid drop, change the existing tile's navigation target in one undoable document edit. Keep its ID, placement, label and appearance. Clicking a connector and then a destination offers the same interaction without dragging.
4. Escape, pointer cancellation or dropping on empty space cancels without changing configuration. Page movement, tile movement, Home movement and connection creation use distinct handles and cannot accidentally become one another.
5. To create a new navigation tile, use the existing navigation item from the library and an available tile cell, then choose or connect its destination. Hold the incomplete operation as a draft; cancellation restores the empty cell. A connection never creates another tile outside the board's grid or overwrites an occupied cell.

Connections are derived from tile or supported top-bar navigation actions. Do not store a second independent list of graph edges. Several buttons can lead to the same page and keep their own IDs. Parallel routes remain separately selectable through the inspector even when their lines are grouped visually.

A line starts at its source button and ends at the destination page's header, with an arrowhead at the destination. Selecting it shows a concrete description such as **Scenes button on Overview opens Scenes, page 5**. It does not mean that every tap on the source page opens the target. Connections do not represent Home Assistant entity relationships.

Changing a line changes the same destination field as the inspector. Removing a navigation button removes its derived connection. If offering removal from a line's inspector, label the operation **Remove navigation button** and make the tile removal undoable; never leave an apparently working button with a hidden missing destination.

## Readability and map arrangement

The default displays routes relevant to the selected page or button. Provide an explicit **All connections** view when users need the whole graph. Use restrained, directed lines and highlight the chosen route. Show Home routes when their source control is selected, rather than drawing a fan of repeated lines back to Home across the entire map. Automatic return and swipe routes are explained in the inspector and reachability result instead of permanently adding more lines.

Use a coarse editor grid for consistent alignment, entirely separate from the tile grid inside each device preview. Dragging a page snaps its outer card to this workspace grid. Prevent overlaps; an occupied drop location gets a clear nearest-free preview or is refused. Mouse, touch and keyboard placement must agree.

Offer an explicit **Arrange from Home** command. Home becomes the first visual row, direct destinations the next, and more distant pages lower down. Assign each page once using shortest directed distance and a deterministic tie-break based on page order. Shared pages remain one node, cross-links remain links, and disconnected pages get a labelled separate area. Cycles cannot duplicate nodes or cause recursion. The command changes workspace coordinates only and is undoable.

Keep manual arrangements stable. Changing Home moves the badge immediately; it does not move every card until the user chooses arrangement. For up to the board's maximum page count, fit-to-view and zoom are sufficient; show a minimap only if later use demonstrates a need. Pan and zoom must not become required skills for ordinary page editing.

## Home, return routes and Back

Keep the draggable Home badge at each page's outer label, with **Set as Home** as a keyboard/touch alternative. It changes `homePageId`, independently of map position and page order. Exactly one page is Home. The physical Home control, Home gesture and automatic return resolve to that page according to the main refactor plan.

A link back to a specific page and a genuine **Back** command have different meanings. A Scenes page reached from both Overview and Lighting has no single parent. Do not generate a Back destination from its map row or from an incoming connection.

Firmware 0.3.0 includes bounded Back navigation. The history holds at most eight stable page IDs, without copies of page or tile configuration. Explicit page links push the page being left; Back pops it, skipping removed pages. Reordering preserves destinations. Home and an external Show page command start a new route. The latter therefore returns to Home when it opens a detail page directly. Repeated cycles discard only the oldest history entry. History is temporary and is not written to preferences or the saved document.

Analyze reachability from the chosen Home using the actual enabled user navigation: explicit buttons, available Home controls/gestures, pagination participation and swipe/page-arrow settings. Automatic return is not a manual way back. Report specific outcomes such as **Only reachable through an automation** or **No manual route Home** instead of treating every page without an incoming tile link as broken. A screen using sequential swipes can be fully usable with no explicit graph edges.

## Local navigation preview

Provide **Try navigation** using the unsaved document and sample or read-only current values. It opens at the selected Home and allows testing actual navigation buttons and the enabled sequential navigation. Use the same filtered sequence, counter and excluded-page behaviour as the device, while allowing explicit links to every configured page. It must not dispatch HA services, tap physical devices, change device settings or mark a layout delivered.

Non-navigation tiles retain a visible preview but their real actions do not run. The simulator uses the same bounded Back rules as firmware and shows the actual footer or top-bar control according to the screen setting. Its route caption is only a review aid. Closing the preview returns to the same editor selection and workspace position.

Use this preview to answer whether a user can get from Home to page 5 and return using the available controls. Validate actual touch handling on hardware separately.

## Small screens and accessibility

At narrow widths, Simple uses a page picker and a directly editable page preview with an accessible tile library. Advanced adds outgoing/incoming destinations as named rows when a map cannot fit. Keep the same page and tile IDs, selection and target picker. Do not shrink a desktop graph until buttons and text are unreadable, and do not require horizontal scrolling to change a destination.

Every drag has an alternative: destination picker for connections, Set as Home for the badge, named move controls for workspace placement and sequence order. Keep source and target names in accessible labels, announce committed changes once, preserve focus after edits and honor reduced-motion preferences. Provide sufficiently large touch targets without enlarging the physical device grid.

## Persistence and migration

The updated editor shows **Update screen to use the new titlebar and layout** for a selected screen whose firmware lacks the new support. **Update screen** opens that screen's existing update flow. Existing supported editing stays available. Once the add-on and screen support the new format and the stored layout is ready, the manager automatically sends and activates it. No separate switch, confirmation, Save or reload is needed.

This automatic activation is independent of Simple/Advanced mode. Keep the user's selected mode throughout the update. A user in Simple immediately gains the supported per-page top-bar and layout features within that familiar editor; Advanced remains optional. Switching back to Simple never downgrades the format or disables supported device features.

Keep this status live in an already-open editor: update required, **Applying new titlebar and layout**, then normal operation after the screen acknowledges the complete configuration. Distinguish waiting for an offline screen or checking its running version from a confirmed need to update. Preserve unsaved edits and the current selection throughout; automatic activation uses saved configuration only. Each physical screen transitions independently, including when updates are weeks apart. See the main plan for conversion failures and synchronization recovery.

Save & send distinguishes **Saved, waiting for screen**, **Applying**, and **Applied**. A failed save retains the draft; a failed delivery retains the successfully saved configuration and retries automatically. Only confirmation of the current saved delivery target may show Applied, even if an older confirmation arrives late. Interrupted delivery can resend the same saved configuration without requiring another edit or Save. If a save response is lost, reconcile the saved revision without replacing newer unsaved browser edits.

Compatibility code lives in the add-on, not in the new firmware. If the screen is updated before its add-on, the screen rejects old-format messages and shows **Configuration problem. Update add-on.** Layout operation resumes automatically once the add-on is updated and delivers a complete supported configuration. Recommend add-on-first updates to avoid that temporary interruption. During changed-configuration delivery the device may briefly show a loading state so it can reuse one configuration store instead of retaining two full layouts. Neither this state nor the mismatch changes the Simple/Advanced editor preference.

The document remains authoritative for pages, tile actions, complete page order, each page's `navigation.excludeFromPagination` flag and `homePageId`. Derive the included sequence instead of persisting a second editable order. Store workspace node positions separately, keyed by stable page ID, with its own revision. Workspace writes never cause firmware sync, change the layout's device revision or display an unsent device-change badge. View mode, zoom, pan and selection can remain per-browser preferences.

Include workspace data in add-on backups. Export it as optional editor metadata, preserving the distinction from executable device configuration. Imports that regenerate page IDs remap workspace positions as well; missing metadata simply generates a new arrangement. Removing a page removes its workspace record; undo restores both. A stale workspace write cannot resurrect a deleted page or overwrite a newer document edit.

Workspace positions for already committed pages may autosave independently. Positions referring to newly created, unsaved pages stay in the draft until the layout is saved. Commit page creation/deletion and its workspace additions/removals through one atomic storage operation, while retaining distinct document/workspace revisions. Check page existence and the expected document context on workspace writes. Shared undo must restore both draft pieces without sending firmware changes for a workspace-only move. A failed or conflicting layout save leaves both draft pieces available for recovery.

The map works with existing numeric navigation tiles through the add-on's stable-ID compatibility adapter. It does not require new firmware merely to show or rearrange editor cards. Saving a new Home target or any newly supported device action still follows per-screen capability gates. A legacy layout awaiting safe page migration remains in the existing supported editing path rather than receiving invented IDs or a guessed grid.

## Implementation and review order

Simple with the new page model is the first complete delivery milestone. Advanced is an additional editor milestone on that same model, not a prerequisite for safe storage migration or page-specific bars. The scope remains both workflows; sequencing keeps device/protocol correctness separate from map interaction work. The concept is not evidence that either milestone already meets acceptance.

1. Validate the concept with representative layouts: sequential pages, Home with room branches, page 1 to page 5, a shared Scenes page, cycles, disconnected automation-only pages, long names and every shipping board shape.
2. Complete stable page/tile IDs, typed navigation targets and the opt-out flag with all-false migration in the main refactor. Keep conversion and mixed-version compatibility tests green.
3. Preserve direct editing in Simple as the default and introduce Advanced as an optional mode, with workspace persistence, selection and derived connections without changing any device behaviour.
4. Add destination editing through connectors, the existing picker, Home designation, the pagination opt-out, undo and cancellation. Use the same document operations across both views and gate new device behaviours on firmware support.
5. Add focused editing, local navigation preview, accessible alternatives and reachability feedback. Test the complete flows rather than individual decorative components.

Acceptance requires that moving a card in the map cannot change the firmware payload; rearranging sequence order cannot change page-ID destinations; changing a connector changes exactly one source action; no operation increases board capacity; and Home designation survives every view, copy, import, deletion and undo operation.

Verify the two-page Simple workflow by placing tiles directly onto both pages and saving without entering Advanced or a focused-editor step. Switch repeatedly between modes with unsaved edits, independent top bars, navigation links and a non-first Home page: the full document and undo history must survive unchanged, and switching alone must not send configuration. A firmware update must activate the new format while Simple remains selected. Remembered Advanced preferences must also survive reconnects and updates.

Switch to Simple while a connection is selected and while a connection drag is in progress. No line or invisible connection hit area may remain. Test workspace autosave and shared undo around an unsaved new page, deletion, a rejected layout save and a stale second browser. Verify unsupported page copies leave the document unchanged.

Before production, exercise mouse, keyboard and touch at desktop, tablet and phone widths. Verify connectors track source buttons after resize, zoom, drag and top-bar changes, and do not steal tile interaction. Build editor assets from source and run the repository checks. The interactive concept is a design discussion surface, not firmware or physical acceptance.

Retain the existing hotfix regression cases for HTTP clipboard fallback, failed-copy manual selection and API-key availability before/after pairing. Exercise the sidebar actions in both modes, including a screen without a stored profile. Check stock and override-enabled standby settings using the same board identity, and verify that none of these device-level changes modifies page configuration.

Verify the opt-out starts unchecked for new and migrated pages. Exercise mixed included/excluded pages, zero and one included page, excluded Home, nested deeplinks, explicit return routes and exclusion of the active page. Check counters and swipes in the simulator and on hardware, unchanged numeric Show page targets, preserved board limits, warning accuracy, copy/import/export/undo, both editor modes and rejection by the old-firmware compatibility path. Existing mockups do not demonstrate this feature until their controls and simulator are updated.

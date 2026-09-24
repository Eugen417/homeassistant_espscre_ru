"""Historic storage/export readers, used only at an explicit import boundary.

Current documents never pass through this module. A future release can retire
this reader with a documented minimum source version and upgrade route without
changing normal saves, state delivery or the firmware.
"""
from copy import deepcopy

from core import header_items, tile_size, validate_layout
from page_layout import APPEARANCE, INTERACTION, FORMAT, LayoutError, _object, new_id, tile_from_fields, validate_document


def _recover_tiles(raw, grid):
    """Salvage stored v1 tiles independently; never relocate an explicit slot.

    The caller has an exact durable backup. Unknown option keys remain in that
    backup and do not prevent known card fields from entering the new document.
    Imports can retain strict validation by not selecting this recovery path.
    """
    _object(raw, {"title", "tiles", "pages", "page_titles", "header", "settings"}, {"title", "tiles"})
    if not isinstance(raw["tiles"], list):
        raise LayoutError("Invalid legacy tiles")
    base = {**deepcopy(raw), "tiles": []}
    validate_layout(base, stored=True, grid=grid)
    kept, dropped = [], []
    for tile in raw["tiles"]:
        try:
            _object(tile, {"entity", "name", "slot", "options"}, {"entity"})
            tile = deepcopy(tile)
            if isinstance(tile.get("options"), dict):
                known = {*APPEARANCE.values(), *INTERACTION.values(), "size"}
                tile["options"] = {key: value for key, value in tile["options"].items() if key in known}
            # The old stored=True escape hatch skips placement checks for
            # unknown options. Strip only unknown keys, then validate normally
            # so one malformed card cannot poison the canonical document.
            validate_layout({**base, "tiles": [*kept, tile]}, grid=grid)
        except (ValueError, TypeError, KeyError, OverflowError):
            item = tile if isinstance(tile, dict) else {}
            dropped.append({"entity": item.get("entity") if isinstance(item.get("entity"), str) else "",
                            "name": item.get("name") if isinstance(item.get("name"), str) else "",
                            "reason": "invalid_legacy_tile"})
        else:
            kept.append(tile)
    return {**base, "tiles": kept}, dropped


def migrate_legacy(raw, grid, id_factory=new_id, *, recover=False):
    """Convert an original v1 payload with a known source grid, or refuse.

    Call before a legacy loader packs missing slots on its default grid. The
    caller retains the original payload when conversion is not lossless.
    """
    dropped = []
    if recover:
        raw, dropped = _recover_tiles(raw, grid)
    _object(raw, {"title", "tiles", "pages", "page_titles", "header", "settings"}, {"title", "tiles"})
    if not isinstance(raw["tiles"], list):
        raise LayoutError("Invalid legacy tiles")
    for tile in raw["tiles"]:
        _object(tile, {"entity", "name", "slot", "options"}, {"entity"})
    legacy = validate_layout(deepcopy(raw), stored=recover, grid=grid)
    used = max((tile["slot"] + grid.cells(tile_size(tile)) for tile in legacy["tiles"]), default=0)
    count = max(1, legacy.get("pages", 1), (used + grid.slots - 1) // grid.slots)
    if count > grid.pages:
        raise LayoutError("Legacy layout exceeds the verified source grid")
    ids = [id_factory() for _ in range(count)]
    titles = legacy.get("page_titles", [])
    pages = []
    for index, page_id in enumerate(ids):
        title = titles[index] if index < len(titles) else ""
        pages.append({
            "id": page_id, "navigation": {"excludeFromPagination": False},
            "topbar": {
                "leading": [{"id": id_factory(), "kind": "home"}],
                "title": {"source": "text", "text": title} if title else {"source": "screen"},
                "trailing": [{"id": id_factory(), **deepcopy(item)} for item in header_items(legacy)],
            }, "tiles": [],
        })
    for tile in legacy["tiles"]:
        pages[tile["slot"] // grid.slots]["tiles"].append(tile_from_fields(tile, grid, ids, id_factory))
    layout = validate_document({"title": legacy["title"], "homePageId": ids[0], "pages": pages}, grid)
    record = {"format": FORMAT, "sourceGrid": {"columns": grid.columns, "rows": grid.rows}, "layout": layout}
    if "settings" in legacy:
        record["settings"] = deepcopy(legacy["settings"])
    if len(titles) > count:
        record["migration"] = {"inactivePageTitles": deepcopy(titles[count:])}
    if dropped:
        record.setdefault("migration", {})["droppedTiles"] = dropped
    return record

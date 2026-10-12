"""Explicit board metadata for existing card/legacy-transport unit fixtures.

These tests describe a known board. Previously they implicitly relied on the
manager's default CYD grid. Supply the diagnostic a known test profile reports
instead. Tests for unknown/pending migrations deliberately do not use this.
"""
from copy import deepcopy


def with_screen_grid(ha):
    from core import discover_screens, shape_of, NAME_SCREEN_LAYOUT
    ha.registry = deepcopy(ha.registry)
    ha.states = deepcopy(ha.states)
    screens = discover_screens(ha.registry, ha.states, ha.devices, ha.areas)
    devices = set()
    for screen in screens:
        device = screen.get('device_id')
        if device in devices or any(item.get('device_id') == device and item.get('original_name') in NAME_SCREEN_LAYOUT
                                    for item in ha.registry):
            continue
        devices.add(device)
        shape = shape_of(screen)
        entity = 'sensor.test_grid_' + screen['id'].replace('.', '_')
        ha.registry.append({'entity_id': entity, 'platform': 'esphome', 'device_id': device, 'original_name': 'Screen layout'})
        ha.states[entity] = {'state': f"{shape['width']}x{shape['height']} {shape['columns']}x{shape['rows']}"}
    return ha


def seed_layout(manager, inbox, layout):
    """Install a historical fixture through the current persistent boundary.

    Card tests intentionally bypass HA capability checks while constructing
    already-saved data. Derived manager.layouts are read-only snapshots.
    """
    from layout_migrations import migrate_legacy
    previous = manager.store.get(inbox)
    record = migrate_legacy(layout, manager.verified_grid(inbox))
    return manager.store.save(inbox, record['layout'], previous['revision'] if previous else None,
                              settings=record.get('settings'))


def edit_layout(manager, inbox, layout):
    """Apply a current editor's structural tile edit, keeping stable page IDs."""
    from page_layout import FORMAT, replace_tiles
    previous = manager.store.get(inbox)
    if previous is None:
        return manager.save(inbox, layout)
    document = replace_tiles(previous, layout)
    document['title'] = layout['title']
    return manager.save_pages(inbox, {'format': FORMAT, 'revision': previous['revision'], 'layout': document})

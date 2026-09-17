"""Curated Material Design Icons for tiles, shared by the editor and both firmware boards.

This is the only list. `python3 tools/generate_icons.py` writes these glyphs into
the three icon fonts of both board profiles and builds the editor font
`static/tile-icons.woff`; `--check` verifies names and codepoints against
fonts/materialdesignicons-webfont.ttf. Firmware 0.2.18+ carries every glyph here.
"""

# (group, ((MDI name, codepoint, label), ...)) in picker order.
GROUPS = (
    ('Lighting', (
        ('lightbulb', 'F0335', 'Light bulb'),
        ('lightbulb-group', 'F1253', 'Lights'),
        ('ceiling-light', 'F0769', 'Ceiling light'),
        ('chandelier', 'F1793', 'Chandelier'),
        ('lamp', 'F06B5', 'Table lamp'),
        ('floor-lamp', 'F08DD', 'Floor lamp'),
        ('desk-lamp', 'F095F', 'Desk lamp'),
        ('wall-sconce-flat', 'F091D', 'Wall light'),
        ('light-recessed', 'F179B', 'Recessed light'),
        ('spotlight-beam', 'F04C9', 'Spotlight'),
        ('led-strip-variant', 'F1051', 'LED strip'),
        ('outdoor-lamp', 'F1054', 'Outdoor light'),
    )),
    ('Rooms', (
        ('home', 'F02DC', 'House'),
        ('sofa', 'F04B9', 'Living room'),
        ('bed', 'F02E3', 'Bedroom'),
        ('silverware-fork-knife', 'F0A70', 'Dining room'),
        ('chef-hat', 'F0B7C', 'Kitchen'),
        ('shower', 'F09A0', 'Shower'),
        ('bathtub', 'F1818', 'Bathroom'),
        ('toilet', 'F09AB', 'Toilet'),
        ('desk', 'F1239', 'Office'),
        ('garage', 'F06D9', 'Garage'),
        ('fireplace', 'F0E2E', 'Fireplace'),
    )),
    ('Climate', (
        ('thermometer', 'F050F', 'Temperature'),
        ('thermostat', 'F0393', 'Thermostat'),
        ('air-conditioner', 'F001B', 'AC'),
        ('radiator', 'F0438', 'Radiator'),
        ('heat-pump', 'F1A43', 'Heat pump'),
        ('fire', 'F0238', 'Heating'),
        ('snowflake', 'F0717', 'Cooling'),
        ('water-percent', 'F058E', 'Humidity'),
        ('fan', 'F0210', 'Fan'),
        ('ceiling-fan', 'F1797', 'Ceiling fan'),
        ('air-purifier', 'F0D44', 'Air purifier'),
        ('molecule-co2', 'F07E4', 'CO₂'),
    )),
    ('Weather', (
        ('weather-sunny', 'F0599', 'Sun'),
        ('weather-partly-cloudy', 'F0595', 'Partly cloudy'),
        ('weather-cloudy', 'F0590', 'Cloudy'),
        ('weather-rainy', 'F0597', 'Rain'),
        ('weather-snowy', 'F0598', 'Snow'),
        ('weather-windy', 'F059D', 'Wind'),
        ('weather-night', 'F0594', 'Night'),
        ('weather-sunset-up', 'F059C', 'Sunrise'),
    )),
    ('Media and music', (
        ('music-note', 'F0387', 'Music'),
        ('music', 'F075A', 'Music notes'),
        ('playlist-music', 'F0CB8', 'Playlist'),
        ('album', 'F0025', 'Album'),
        ('record-player', 'F099A', 'Record player'),
        ('spotify', 'F04C7', 'Spotify'),
        ('radio', 'F0439', 'Radio'),
        ('podcast', 'F0994', 'Podcast'),
        ('speaker', 'F04C3', 'Speaker'),
        ('speaker-multiple', 'F0D38', 'Speakers'),
        ('soundbar', 'F17DB', 'Soundbar'),
        ('headphones', 'F02CB', 'Headphones'),
        ('piano', 'F067D', 'Piano'),
        ('guitar-acoustic', 'F0771', 'Guitar'),
        ('television', 'F0502', 'TV'),
        ('projector', 'F042E', 'Projector'),
        ('movie-open', 'F0FCE', 'Movie'),
        ('cast', 'F0118', 'Cast'),
        ('gamepad-variant', 'F0297', 'Game console'),
        ('remote-tv', 'F0EC5', 'Remote control'),
    )),
    ('Security', (
        ('lock', 'F033E', 'Lock'),
        ('lock-open-variant', 'F0FC6', 'Lock open'),
        ('shield-home', 'F068A', 'Alarm'),
        ('alarm-light', 'F078F', 'Alarm light'),
        ('bell', 'F009A', 'Bell'),
        ('doorbell', 'F12E6', 'Doorbell'),
        ('cctv', 'F07AE', 'Camera'),
        ('motion-sensor', 'F0D91', 'Motion sensor'),
        ('door-closed', 'F081B', 'Door'),
        ('door-open', 'F081C', 'Door open'),
        ('window-closed-variant', 'F11DB', 'Window'),
        ('smoke-detector', 'F0392', 'Smoke detector'),
        ('water-alert', 'F1502', 'Water leak'),
    )),
    ('Appliances', (
        ('washing-machine', 'F072A', 'Washing machine'),
        ('tumble-dryer', 'F0917', 'Dryer'),
        ('dishwasher', 'F0AAC', 'Dishwasher'),
        ('fridge', 'F0290', 'Fridge'),
        ('stove', 'F04DE', 'Stove'),
        ('microwave', 'F0C99', 'Microwave'),
        ('toaster-oven', 'F0CD3', 'Oven'),
        ('coffee-maker', 'F109F', 'Coffee'),
        ('kettle', 'F05FA', 'Kettle'),
        ('robot-vacuum', 'F070D', 'Robot vacuum'),
        ('robot-mower', 'F11F7', 'Robot mower'),
        ('printer-3d', 'F042B', '3D printer'),
        ('power-plug', 'F06A5', 'Plug'),
        ('power-socket-eu', 'F07E7', 'Socket'),
        ('laptop', 'F0322', 'Laptop'),
        ('router-wireless', 'F0469', 'Router'),
    )),
    ('Energy', (
        ('flash', 'F0241', 'Power'),
        ('solar-power', 'F0A72', 'Solar power'),
        ('solar-panel', 'F0D9B', 'Solar panels'),
        ('home-battery', 'F1901', 'Home battery'),
        ('battery-high', 'F12A3', 'Battery'),
        ('ev-station', 'F05F1', 'Charging station'),
        ('car-electric', 'F0B6C', 'Electric car'),
        ('meter-electric', 'F1A57', 'Power meter'),
        ('meter-gas', 'F1A59', 'Gas meter'),
        ('water', 'F058C', 'Water'),
        ('water-boiler', 'F0F92', 'Boiler'),
        ('gauge', 'F029A', 'Meter'),
    )),
    ('Window coverings', (
        ('window-shutter', 'F111C', 'Shutter'),
        ('window-shutter-open', 'F111E', 'Shutter open'),
        ('blinds', 'F00AC', 'Blinds'),
        ('blinds-open', 'F1011', 'Blinds open'),
        ('roller-shade-closed', 'F1A6C', 'Roller blind'),
        ('curtains', 'F1846', 'Curtains'),
        ('curtains-closed', 'F1847', 'Curtains closed'),
        ('awning-outline', 'F1B88', 'Awning'),
    )),
    ('Garden and pets', (
        ('flower', 'F024A', 'Flower'),
        ('tree', 'F0531', 'Tree'),
        ('sprout', 'F0E66', 'Plant'),
        ('grass', 'F1510', 'Lawn'),
        ('sprinkler', 'F105F', 'Sprinkler'),
        ('pool', 'F0606', 'Pool'),
        ('grill', 'F0E45', 'Barbecue'),
        ('mailbox', 'F06EE', 'Mailbox'),
        ('trash-can', 'F0A79', 'Trash'),
        ('recycle', 'F044C', 'Recycling'),
        ('dog', 'F0A43', 'Dog'),
        ('cat', 'F011B', 'Cat'),
    )),
    ('People and travel', (
        ('account', 'F0004', 'Person'),
        ('account-group', 'F0849', 'Family'),
        ('account-child', 'F0A89', 'Child'),
        ('home-account', 'F0826', 'Home'),
        ('sleep', 'F04B2', 'Sleeping'),
        ('run', 'F070E', 'Exercising'),
        ('briefcase', 'F00D6', 'Work'),
        ('car', 'F010B', 'Car'),
        ('bike', 'F00A3', 'Bike'),
        ('airplane', 'F001D', 'Vacation'),
        ('map-marker', 'F034E', 'Location'),
    )),
    ('Other', (
        ('power', 'F0425', 'On/off'),
        ('toggle-switch', 'F0521', 'Switch'),
        ('gesture-tap-button', 'F12A8', 'Button'),
        ('play', 'F040A', 'Start'),
        ('script-text', 'F0BC2', 'Script'),
        ('palette', 'F03D8', 'Scene'),
        ('alarm', 'F0020', 'Alarm clock'),
        ('clock-outline', 'F0150', 'Clock'),
        ('timer-outline', 'F051B', 'Timer'),
        ('calendar', 'F00ED', 'Calendar'),
        ('bell-ring', 'F009E', 'Notification'),
        ('heart', 'F02D1', 'Favorite'),
        ('alert-outline', 'F002A', 'Warning'),
        ('cog', 'F0493', 'Settings'),
        ('broom', 'F00E2', 'Cleaning'),
    )),
)

# Glyphs the firmware draws itself (weather conditions, sun, checkmark, direct controls) that the picker does not offer.
FIXED = (
    ('alert-circle-outline', 'F05D6'),
    ('check', 'F012C'),
    # Direct controls on wide cards (firmware 0.2.19+).
    ('pause', 'F03E4'),
    ('stop', 'F04DB'),
    ('skip-next', 'F04AD'),
    ('skip-previous', 'F04AE'),
    ('volume-high', 'F057E'),
    ('volume-off', 'F0581'),
    ('arrow-up', 'F005D'),
    ('arrow-down', 'F0045'),
    ('arrow-expand-horizontal', 'F084E'),
    ('arrow-collapse-horizontal', 'F084C'),
    ('home-map-marker', 'F05F8'),
    ('plus', 'F0415'),
    ('minus', 'F0374'),
    ('chevron-left', 'F0141'),
    ('chevron-right', 'F0142'),
    ('close', 'F0156'),
    # Back button of every overlay (firmware 0.2.37+).
    ('arrow-left', 'F004D'),
    ('sun-snowflake-variant', 'F1A79'),
    ('thermostat-auto', 'F1B17'),
    ('weather-fog', 'F0591'),
    ('weather-hail', 'F0592'),
    ('weather-lightning', 'F0593'),
    ('weather-lightning-rainy', 'F067E'),
    ('weather-pouring', 'F0596'),
    ('weather-snowy-rainy', 'F067F'),
    ('weather-sunset-down', 'F059B'),
    # "More" key on the CYD climate card: fan and swing (firmware 0.2.39+).
    ('dots-horizontal', 'F01D8'),
    # Swing row on the Guition climate card, Home Assistant's icon for swing modes (firmware 0.2.40+).
    ('arrow-oscillating', 'F1C91'),
    # The settings page on the screen itself (firmware 0.2.44+): its menu and the restart row.
    ('monitor', 'F0379'),
    ('information-outline', 'F02FD'),
    ('restart', 'F0709'),
    # A light that is off, as Home Assistant shows one without an icon of its own (firmware 0.2.53+).
    ('lightbulb-off', 'F0E4F'),
)

# Pickable icons by name, and every glyph the firmware fonts contain.
ICONS = {name: (codepoint, label) for _, icons in GROUPS for name, codepoint, label in icons}
GLYPHS = {**{name: codepoint for name, (codepoint, _) in ICONS.items()}, **dict(FIXED)}

# Mirrors runtime_tiles::icon_for() so the editor mockup shows what the screen draws.
DEFAULTS = {'light': 'lightbulb', 'climate': 'air-conditioner', 'vacuum': 'robot-vacuum', 'fan': 'fan',
            'cover': 'window-shutter', 'scene': 'sofa', 'script': 'sofa', 'sensor': 'gauge', 'binary_sensor': 'gauge',
            'timer': 'timer-outline', 'person': 'account', 'camera': 'cctv', 'image': 'cctv', 'screen': 'clock-outline'}
# The cards the screen brings itself: one icon per entity, not per domain.
BUILTIN_TILES = {'screen.clock': 'clock-outline', 'screen.settings': 'cog'}
FALLBACK = 'power'
CONTROL_GLYPHS = ('play', 'pause', 'stop', 'skip-next', 'skip-previous', 'volume-high', 'volume-off', 'arrow-up', 'arrow-down',
                  'arrow-expand-horizontal', 'arrow-collapse-horizontal', 'home-map-marker', 'plus', 'minus', 'chevron-left',
                  'chevron-right', 'close', 'power', 'fire', 'snowflake')
WEATHER = {'sunny': 'weather-sunny', 'clear-night': 'weather-night', 'cloudy': 'weather-cloudy',
           'partlycloudy': 'weather-partly-cloudy', 'rainy': 'weather-rainy', 'pouring': 'weather-pouring',
           'snowy': 'weather-snowy', 'snowy-rainy': 'weather-snowy-rainy', 'fog': 'weather-fog', 'hail': 'weather-hail',
           'lightning': 'weather-lightning', 'lightning-rainy': 'weather-lightning-rainy', 'windy': 'weather-windy',
           'windy-variant': 'weather-windy', 'exceptional': 'alert-circle-outline'}

def ha_icon(attributes):
    """Codepoint of Home Assistant's own `mdi:` icon when the firmware carries it, else None."""
    icon = attributes.get('icon') if isinstance(attributes, dict) else None
    return GLYPHS.get(icon[4:]) if isinstance(icon, str) and icon.startswith('mdi:') else None

def editor():
    """Picker groups plus what the mockup needs to predict the screen's own icons."""
    return {'groups': [{'label': group, 'icons': [{'name': name, 'cp': codepoint, 'label': label} for name, codepoint, label in icons]}
                       for group, icons in GROUPS],
            'defaults': {domain: GLYPHS[name] for domain, name in DEFAULTS.items()}, 'fallback': GLYPHS[FALLBACK],
            'builtin': {entity: GLYPHS[name] for entity, name in BUILTIN_TILES.items()},
            'weather': {state: GLYPHS[name] for state, name in WEATHER.items()},
            'sun': {'above_horizon': GLYPHS['weather-sunset-down'], 'below_horizon': GLYPHS['weather-sunset-up']},
            # Glyphs of the direct controls on wide cards, so the mockup previews them.
            'controls': {name: GLYPHS[name] for name in CONTROL_GLYPHS}}

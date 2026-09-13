"""Curated Material Design Icons for tiles, shared by the editor and both firmware boards.

This is the only list. `python3 tools/generate_icons.py` writes these glyphs into
the three icon fonts of both board profiles and builds the editor font
`static/tile-icons.woff`; `--check` verifies names and codepoints against
fonts/materialdesignicons-webfont.ttf. Firmware 0.2.18+ carries every glyph here.
"""

# (group, ((MDI name, codepoint, label), ...)) in picker order.
GROUPS = (
    ('Verlichting', (
        ('lightbulb', 'F0335', 'Lamp'),
        ('lightbulb-group', 'F1253', 'Lampen'),
        ('ceiling-light', 'F0769', 'Plafondlamp'),
        ('chandelier', 'F1793', 'Kroonluchter'),
        ('lamp', 'F06B5', 'Tafellamp'),
        ('floor-lamp', 'F08DD', 'Vloerlamp'),
        ('desk-lamp', 'F095F', 'Bureaulamp'),
        ('wall-sconce-flat', 'F091D', 'Wandlamp'),
        ('light-recessed', 'F179B', 'Inbouwspot'),
        ('spotlight-beam', 'F04C9', 'Spot'),
        ('led-strip-variant', 'F1051', 'Ledstrip'),
        ('outdoor-lamp', 'F1054', 'Buitenlamp'),
    )),
    ('Ruimtes', (
        ('home', 'F02DC', 'Huis'),
        ('sofa', 'F04B9', 'Woonkamer'),
        ('bed', 'F02E3', 'Slaapkamer'),
        ('silverware-fork-knife', 'F0A70', 'Eetkamer'),
        ('chef-hat', 'F0B7C', 'Keuken'),
        ('shower', 'F09A0', 'Douche'),
        ('bathtub', 'F1818', 'Badkamer'),
        ('toilet', 'F09AB', 'Toilet'),
        ('desk', 'F1239', 'Werkkamer'),
        ('garage', 'F06D9', 'Garage'),
        ('fireplace', 'F0E2E', 'Open haard'),
    )),
    ('Klimaat', (
        ('thermometer', 'F050F', 'Temperatuur'),
        ('thermostat', 'F0393', 'Thermostaat'),
        ('air-conditioner', 'F001B', 'Airco'),
        ('radiator', 'F0438', 'Radiator'),
        ('heat-pump', 'F1A43', 'Warmtepomp'),
        ('fire', 'F0238', 'Verwarming'),
        ('snowflake', 'F0717', 'Koelen'),
        ('water-percent', 'F058E', 'Luchtvochtigheid'),
        ('fan', 'F0210', 'Ventilator'),
        ('ceiling-fan', 'F1797', 'Plafondventilator'),
        ('air-purifier', 'F0D44', 'Luchtreiniger'),
        ('molecule-co2', 'F07E4', 'CO₂'),
    )),
    ('Weer', (
        ('weather-sunny', 'F0599', 'Zon'),
        ('weather-partly-cloudy', 'F0595', 'Half bewolkt'),
        ('weather-cloudy', 'F0590', 'Bewolkt'),
        ('weather-rainy', 'F0597', 'Regen'),
        ('weather-snowy', 'F0598', 'Sneeuw'),
        ('weather-windy', 'F059D', 'Wind'),
        ('weather-night', 'F0594', 'Nacht'),
        ('weather-sunset-up', 'F059C', 'Zonsopkomst'),
    )),
    ('Media en muziek', (
        ('music-note', 'F0387', 'Muziek'),
        ('music', 'F075A', 'Muzieknoten'),
        ('playlist-music', 'F0CB8', 'Afspeellijst'),
        ('album', 'F0025', 'Album'),
        ('record-player', 'F099A', 'Platenspeler'),
        ('spotify', 'F04C7', 'Spotify'),
        ('radio', 'F0439', 'Radio'),
        ('podcast', 'F0994', 'Podcast'),
        ('speaker', 'F04C3', 'Speaker'),
        ('speaker-multiple', 'F0D38', 'Speakers'),
        ('soundbar', 'F17DB', 'Soundbar'),
        ('headphones', 'F02CB', 'Koptelefoon'),
        ('piano', 'F067D', 'Piano'),
        ('guitar-acoustic', 'F0771', 'Gitaar'),
        ('television', 'F0502', 'Televisie'),
        ('projector', 'F042E', 'Beamer'),
        ('movie-open', 'F0FCE', 'Film'),
        ('cast', 'F0118', 'Casten'),
        ('gamepad-variant', 'F0297', 'Gameconsole'),
        ('remote-tv', 'F0EC5', 'Afstandsbediening'),
    )),
    ('Beveiliging', (
        ('lock', 'F033E', 'Slot'),
        ('lock-open-variant', 'F0FC6', 'Slot open'),
        ('shield-home', 'F068A', 'Alarm'),
        ('alarm-light', 'F078F', 'Alarmlicht'),
        ('bell', 'F009A', 'Bel'),
        ('doorbell', 'F12E6', 'Deurbel'),
        ('cctv', 'F07AE', 'Camera'),
        ('motion-sensor', 'F0D91', 'Bewegingsmelder'),
        ('door-closed', 'F081B', 'Deur'),
        ('door-open', 'F081C', 'Deur open'),
        ('window-closed-variant', 'F11DB', 'Raam'),
        ('smoke-detector', 'F0392', 'Rookmelder'),
        ('water-alert', 'F1502', 'Waterlek'),
    )),
    ('Apparaten', (
        ('washing-machine', 'F072A', 'Wasmachine'),
        ('tumble-dryer', 'F0917', 'Droger'),
        ('dishwasher', 'F0AAC', 'Vaatwasser'),
        ('fridge', 'F0290', 'Koelkast'),
        ('stove', 'F04DE', 'Fornuis'),
        ('microwave', 'F0C99', 'Magnetron'),
        ('toaster-oven', 'F0CD3', 'Oven'),
        ('coffee-maker', 'F109F', 'Koffie'),
        ('kettle', 'F05FA', 'Waterkoker'),
        ('robot-vacuum', 'F070D', 'Robotstofzuiger'),
        ('robot-mower', 'F11F7', 'Robotmaaier'),
        ('printer-3d', 'F042B', '3D-printer'),
        ('power-plug', 'F06A5', 'Stekker'),
        ('power-socket-eu', 'F07E7', 'Stopcontact'),
        ('laptop', 'F0322', 'Laptop'),
        ('router-wireless', 'F0469', 'Router'),
    )),
    ('Energie', (
        ('flash', 'F0241', 'Stroom'),
        ('solar-power', 'F0A72', 'Zonne-energie'),
        ('solar-panel', 'F0D9B', 'Zonnepanelen'),
        ('home-battery', 'F1901', 'Thuisbatterij'),
        ('battery-high', 'F12A3', 'Batterij'),
        ('ev-station', 'F05F1', 'Laadpaal'),
        ('car-electric', 'F0B6C', 'Elektrische auto'),
        ('meter-electric', 'F1A57', 'Stroommeter'),
        ('meter-gas', 'F1A59', 'Gasmeter'),
        ('water', 'F058C', 'Water'),
        ('water-boiler', 'F0F92', 'Boiler'),
        ('gauge', 'F029A', 'Meter'),
    )),
    ('Zonwering', (
        ('window-shutter', 'F111C', 'Rolluik'),
        ('window-shutter-open', 'F111E', 'Rolluik open'),
        ('blinds', 'F00AC', 'Jaloezie'),
        ('blinds-open', 'F1011', 'Jaloezie open'),
        ('roller-shade-closed', 'F1A6C', 'Rolgordijn'),
        ('curtains', 'F1846', 'Gordijnen'),
        ('curtains-closed', 'F1847', 'Gordijnen dicht'),
        ('awning-outline', 'F1B88', 'Zonnescherm'),
    )),
    ('Tuin en huisdieren', (
        ('flower', 'F024A', 'Bloem'),
        ('tree', 'F0531', 'Boom'),
        ('sprout', 'F0E66', 'Plant'),
        ('grass', 'F1510', 'Gazon'),
        ('sprinkler', 'F105F', 'Sproeier'),
        ('pool', 'F0606', 'Zwembad'),
        ('grill', 'F0E45', 'Barbecue'),
        ('mailbox', 'F06EE', 'Brievenbus'),
        ('trash-can', 'F0A79', 'Afval'),
        ('recycle', 'F044C', 'Recycling'),
        ('dog', 'F0A43', 'Hond'),
        ('cat', 'F011B', 'Kat'),
    )),
    ('Mensen en onderweg', (
        ('account', 'F0004', 'Persoon'),
        ('account-group', 'F0849', 'Gezin'),
        ('account-child', 'F0A89', 'Kind'),
        ('home-account', 'F0826', 'Thuis'),
        ('sleep', 'F04B2', 'Slapen'),
        ('run', 'F070E', 'Sporten'),
        ('briefcase', 'F00D6', 'Werk'),
        ('car', 'F010B', 'Auto'),
        ('bike', 'F00A3', 'Fiets'),
        ('airplane', 'F001D', 'Vakantie'),
        ('map-marker', 'F034E', 'Locatie'),
    )),
    ('Overig', (
        ('power', 'F0425', 'Aan/uit'),
        ('toggle-switch', 'F0521', 'Schakelaar'),
        ('gesture-tap-button', 'F12A8', 'Knop'),
        ('play', 'F040A', 'Start'),
        ('script-text', 'F0BC2', 'Script'),
        ('palette', 'F03D8', 'Scène'),
        ('alarm', 'F0020', 'Wekker'),
        ('clock-outline', 'F0150', 'Klok'),
        ('timer-outline', 'F051B', 'Timer'),
        ('calendar', 'F00ED', 'Agenda'),
        ('bell-ring', 'F009E', 'Melding'),
        ('heart', 'F02D1', 'Favoriet'),
        ('alert-outline', 'F002A', 'Waarschuwing'),
        ('cog', 'F0493', 'Instellingen'),
        ('broom', 'F00E2', 'Schoonmaken'),
    )),
)

# Glyphs the firmware draws itself (weather conditions, sun, checkmark) that the picker does not offer.
FIXED = (
    ('alert-circle-outline', 'F05D6'),
    ('check', 'F012C'),
    ('weather-fog', 'F0591'),
    ('weather-hail', 'F0592'),
    ('weather-lightning', 'F0593'),
    ('weather-lightning-rainy', 'F067E'),
    ('weather-pouring', 'F0596'),
    ('weather-snowy-rainy', 'F067F'),
    ('weather-sunset-down', 'F059B'),
)

# Pickable icons by name, and every glyph the firmware fonts contain.
ICONS = {name: (codepoint, label) for _, icons in GROUPS for name, codepoint, label in icons}
GLYPHS = {**{name: codepoint for name, (codepoint, _) in ICONS.items()}, **dict(FIXED)}

# Mirrors runtime_tiles::icon_for() so the editor mockup shows what the screen draws.
DEFAULTS = {'light': 'lightbulb', 'climate': 'air-conditioner', 'vacuum': 'robot-vacuum', 'fan': 'fan',
            'cover': 'window-shutter', 'scene': 'sofa', 'script': 'sofa', 'sensor': 'gauge', 'binary_sensor': 'gauge',
            'timer': 'timer-outline', 'person': 'account', 'screen': 'clock-outline'}
FALLBACK = 'power'
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
            'weather': {state: GLYPHS[name] for state, name in WEATHER.items()},
            'sun': {'above_horizon': GLYPHS['weather-sunset-down'], 'below_horizon': GLYPHS['weather-sunset-up']}}

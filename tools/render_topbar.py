"""Render the real top bar code (runtime_tiles::draw_header) for both boards on this computer.

Builds a small ESPHome host project (SDL2 display, LVGL 9.5 as the screens run it) with the
fonts copied verbatim from both board profiles, draws a set of scenarios through the firmware's
own drawing code and saves LVGL snapshots as PNG plus one contact sheet: Guition at 2x, CYD at
3x, nearest neighbour, so every pixel shows. No screen, Home Assistant or flash is involved.

Needs the ESPHome CLI (2026.6.2) and SDL2 (`sdl2-config` on PATH, e.g. `brew install sdl2`).
Output: .esphome/render-topbar/out/*.png and sheet.png (or --out).
"""
import argparse
import os
import re
import shutil
import struct
import subprocess
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / '.esphome' / 'render-topbar'
PROFILES = {'guition': 'guition-4848s040.yaml', 'cyd': 'home-like-2432s028.yaml'}
FONTS = ('headline', 'time_label', 'sublabel_big', 'label', 'materialdesign_icons', 'materialdesign_icons_mini')
# Hardware headers the host cannot compile; the top bar needs none of them.
SKIP = {'backlight_fade.h', 'guition_diagnostics.h', '__pycache__'}
# Header band per board: page width and height above the tiles.
BANDS = {'guition': (480, 72), 'cyd': (320, 40)}

# (slug, title, items) with items as C++ expressions of the render lambda's helpers.
SCENARIOS = (
    ('1-klok', 'Studio 1', ['builtin(Kind::clock)']),
    ('2-mockup', 'Woonkamer', ['text(0xF050F, "—")', 'text(0xF050F, "21,3 °C")', 'text(0xF058E, "—")', 'ago(0xF0004, 26 * 3600)']),
    ('3-deur-alarm', 'Studio 1', ['text(0xF081C, "Open", 0xFFB300)', 'text(0xF068A, "Afwezig", 0x43A047)', 'builtin(Kind::clock)']),
    ('4-analoog-datum', 'Keuken', ['text(0xF050F, "23,5 °C")', 'builtin(Kind::date)', 'builtin(Kind::analog)']),
    ('5-thuis-weer-stroom', 'Studio 1', ['text(0xF0849, "4")', 'text(0xF0595, "22 °C")', 'text(0xF0241, "1.249 W")', 'builtin(Kind::clock)']),
    ('6-te-vol', 'Woonkamer beneden', ['text(0xF050F, "21,3 °C")', 'text(0xF058E, "48%")', 'ago(0xF0D91, 5 * 60)', 'text(0xF0241, "78,0 W")', 'builtin(Kind::analog)', 'builtin(Kind::clock)']),
    ('7-mensen', 'Studio 1', ['text(0xF0004, "Weg")', 'text(0xF0849, "4")', 'builtin(Kind::analog)', 'builtin(Kind::clock)']),
)

def font_blocks(board):
    """The profile's font entries, ids suffixed per board and file paths made absolute."""
    text = (ROOT / PROFILES[board]).read_text()
    fonts = text[text.index('\nfont:\n'):]
    out = ''
    for font_id in FONTS:
        m = re.search(r'\n  - file: [^\n]+\n    id: ' + font_id + r'\n(?:    [^\n]*\n|      [^\n]*\n|             [^\n]*\n)*', fonts)
        if not m:
            raise SystemExit(f'Font {font_id} not found in {PROFILES[board]}')
        block = m.group(0).replace(f'id: {font_id}\n', f'id: {font_id}_{board}\n')
        block = re.sub(r'file: ["\']fonts/([^"\']+)["\']', lambda f: f'file: "{ROOT}/fonts/{f[1]}"', block)
        out += block.replace('&tile_icons', f'&tile_icons_{board}').replace('*tile_icons\n', f'*tile_icons_{board}\n')
    return out

def band(board, y):
    width, height = BANDS[board]
    margin, top = (16, 22) if board == 'guition' else (11, 14)
    tile_y, tile_w, tile_h, col2, radius = (72, 218, 108, 246, 22) if board == 'guition' else (40, 147, 52, 164, 18)
    return f'''        - obj:
            id: {board}_page
            x: 0
            y: {y}
            width: {width}
            height: {height + tile_h + margin}
            bg_color: 0xE7E7E7
            bg_opa: COVER
            border_width: 0
            radius: 0
            pad_all: 0
            scrollable: false
            widgets:
              - label:
                  id: {board}_room
                  x: {margin}
                  y: {top}
                  text_font: headline_{board}
                  text_color: 0x1B1B1B
              - label:
                  id: {board}_time
                  align: TOP_RIGHT
                  x: -{margin}
                  y: {top}
                  text: ""
                  text_font: time_label_{board}
              - obj:
                  x: {margin if board == 'guition' else 9}
                  y: {tile_y}
                  width: {tile_w}
                  height: {tile_h}
                  radius: {radius}
                  border_width: 1
                  border_color: 0xDDDDDD
              - obj:
                  x: {col2}
                  y: {tile_y}
                  width: {tile_w}
                  height: {tile_h}
                  radius: {radius}
                  border_width: 1
                  border_color: 0xDDDDDD
'''

def render_lambda(out):
    scenarios = ',\n'.join(f'  {{"{slug}", "{title}", {{{", ".join(items)}}}}}' for slug, title, items in SCENARIOS)
    return f'''using namespace runtime_tiles;
using header_bar::Kind;
static const std::string OUT = "{out}/";
// A fixed moment, 14 september 2026 18:04 in the Netherlands, so renders compare between runs.
static const int64_t NOW = 1789401840;
auto fixed = esphome::ESPTime::from_epoch_local(NOW);
now_time = [fixed]() {{ return fixed; }};
enabled = true;
screen_settings::current.clock_24h = 1;
auto text = [](uint32_t icon, const char *value, uint32_t color = 0) {{
  header_bar::Item item; item.kind = Kind::text; item.icon = icon; item.text = value;
  if (color) {{ item.color = color; item.has_color = true; }}
  return item;
}};
auto ago = [](uint32_t icon, int64_t seconds) {{ header_bar::Item item; item.kind = Kind::ago; item.icon = icon; item.epoch = NOW - seconds; return item; }};
auto builtin = [](Kind kind) {{ header_bar::Item item; item.kind = kind; return item; }};
struct Scenario {{ const char *slug, *title; std::vector<header_bar::Item> items; }};
std::vector<Scenario> scenarios = {{
{scenarios}
}};
struct Board {{ const char *name; lv_obj_t *page, *room, *time; const lv_font_t *text_font, *icon_font; }};
std::vector<Board> boards = {{
  {{"guition", id(guition_page), id(guition_room), id(guition_time), id(sublabel_big_guition)->get_lv_font(), id(materialdesign_icons_mini_guition)->get_lv_font()}},
  {{"cyd", id(cyd_page), id(cyd_room), id(cyd_time), id(sublabel_big_cyd)->get_lv_font(), id(materialdesign_icons_mini_cyd)->get_lv_font()}},
}};
for (auto &scenario : scenarios) {{
  for (auto &board : boards) {{
    // A fresh bar per board: the runtime keeps one set of objects.
    if (header_root) {{ lv_obj_delete(header_root); header_root = nullptr; header_ring = nullptr; header_hands = {{}}; header_slots = {{}}; header_dial_key = -1; }}
    room_label = board.room; time_label = board.time; header_text_font = board.text_font; header_icon_font = board.icon_font;
    lv_label_set_long_mode(board.room, LV_LABEL_LONG_WRAP);
    lv_obj_set_size(board.room, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(board.room, scenario.title);
    header = header_bar::Bar{{}};
    for (auto &item : scenario.items) header.items[header.count++] = item;
    header.received = true;
    for (int pass = 0; pass < 2; ++pass) {{ lv_obj_update_layout(board.page); draw_header(true); }}
    lv_obj_update_layout(board.page);
    lv_draw_buf_t *buf = lv_snapshot_take(board.page, LV_COLOR_FORMAT_RGB888);
    std::string path = OUT + scenario.slug + "-" + board.name + ".ppm";
    FILE *f = fopen(path.c_str(), "wb");
    fprintf(f, "P6\\n%d %d\\n255\\n", (int)buf->header.w, (int)buf->header.h);
    for (int y = 0; y < (int)buf->header.h; ++y) {{
      const uint8_t *row = buf->data + y * buf->header.stride;
      for (int x = 0; x < (int)buf->header.w; ++x) {{ uint8_t px[3] = {{row[x * 3 + 2], row[x * 3 + 1], row[x * 3]}}; fwrite(px, 1, 3, f); }}
    }}
    fclose(f);
    lv_draw_buf_destroy(buf);
  }}
}}
ESP_LOGI("render", "klaar");
fflush(stdout);
exit(0);
'''

def project(out):
    lam = '\n'.join(('          ' + line) if line else '' for line in render_lambda(out).splitlines())
    return f'''esphome:
  name: render-topbar
  platformio_options:
    build_flags:
      - -DLV_USE_SNAPSHOT=1
  on_boot:
    priority: -100
    then:
      - lambda: |-
{lam}

host:

external_components:
  - source:
      type: local
      path: {WORK}/components
    components: [smart_display]

logger:
  level: INFO

# Sets the process time zone the way the screens get it; the renders use a fixed moment.
time:
  - platform: host
    timezone: Europe/Amsterdam

api:
  homeassistant_services: true
  reboot_timeout: 0s

text:
  - platform: smart_display
    name: "Tegelinstellingen"

display:
  - platform: sdl
    id: sdl_display
    dimensions:
      width: 480
      height: 480
    auto_clear_enabled: false
    update_interval: never

font:{font_blocks('guition')}{font_blocks('cyd')}
lvgl:
  displays: sdl_display
  pages:
    - id: render_page
      pad_all: 0
      scrollable: false
      widgets:
        # Never shown: they make ESPHome compile the widgets the component code creates.
        - slider:
            hidden: true
        - switch:
            hidden: true
        - spinner:
            hidden: true
            spin_time: 1s
            arc_length: 60deg
        - line:
            hidden: true
            points:
              - 0, 0
              - 1, 1
{band('guition', 0)}{band('cyd', 220)}
script:
  - id: action_seed
    then:
      - homeassistant.action:
          action: light.turn_on
'''

def png(path, width, height, rgb):
    raw = b''.join(b'\0' + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    chunk = lambda kind, data: struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xFFFFFFFF)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

def read_ppm(path):
    data = path.read_bytes()
    magic, width, height, depth, pixels = re.match(rb'(P6)\s+(\d+)\s+(\d+)\s+(\d+)\s(.*)', data, re.S).groups()
    return int(width), int(height), pixels

def crop_scale(image, width, height, scale):
    w, _, rgb = image
    out = bytearray()
    for y in range(height * scale):
        row = rgb[(y // scale) * w * 3:(y // scale) * w * 3 + width * 3]
        out += b''.join(row[x * 3:x * 3 + 3] * scale for x in range(width))
    return bytes(out)

def sheet(out):
    """Header bands of every scenario: Guition 2x above CYD 3x, 12 px white between."""
    width, gap = 960, 12
    rows = []
    for slug, _, _ in SCENARIOS:
        for board, scale in (('guition', 2), ('cyd', 3)):
            image = read_ppm(out / f'{slug}-{board}.ppm')
            band_w, band_h = BANDS[board]
            png(out / f'{slug}-{board}.png', image[0], image[1], image[2])
            rows.append((crop_scale(image, band_w, band_h, scale), band_h * scale))
        rows.append((bytes([255]) * width * 3 * gap * 2, gap * 2))
    height = sum(h for _, h in rows)
    png(out / 'sheet.png', width, height, b''.join(data for data, _ in rows))
    return out / 'sheet.png'

def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--out', type=Path, default=WORK / 'out')
    parser.add_argument('--esphome', default=os.environ.get('ESPHOME') or shutil.which('esphome') or str(Path.home() / '.local/pipx/venvs/esphome/bin/esphome'))
    args = parser.parse_args()
    if not shutil.which('sdl2-config'):
        raise SystemExit('SDL2 ontbreekt: installeer het eerst (bijvoorbeeld brew install sdl2).')
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob('*.p[np][gm]'):
        old.unlink()
    components = WORK / 'components' / 'smart_display'
    shutil.rmtree(components, ignore_errors=True)
    components.mkdir(parents=True)
    for source in (ROOT / 'components/smart_display').iterdir():
        if source.name not in SKIP:
            (components / source.name).symlink_to(source)
    config = WORK / 'render-topbar.yaml'
    config.write_text(project(out))
    build = subprocess.run([args.esphome, 'compile', str(config)], capture_output=True, text=True)
    if build.returncode:
        sys.stdout.write(build.stdout[-4000:] + build.stderr[-2000:])
        raise SystemExit('Build mislukt.')
    program = next(WORK.glob('.esphome/build/render-topbar/.pioenvs/render-topbar/program'), None)
    if not program:
        raise SystemExit('Geen host-programma gevonden na de build.')
    subprocess.run([str(program)], capture_output=True, timeout=60, check=True)
    print(sheet(out))

if __name__ == '__main__':
    main()

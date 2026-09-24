"""Render host-only tall-tile design proposals with LVGL and real board metrics.

No firmware files, storage, or device configuration are changed. Artwork is an
illustrative LVGL cover placeholder, not a decoded media thumbnail.
Run with the ESPHome Python environment; needs SDL2 and Pillow.
"""
import json, os, subprocess, sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import profiles
WORK=ROOT/'.esphome/tall-design-review/concepts'
OUT=WORK/'out'
BOARDS=('cyd','guition','waveshare43','waveshare7','jc8012p4a1','waveshare43_portrait')
SCENES=('media','climate','tall','light','vacuum','media-none','media-volume','climate-modes','climate-none')

def project():
    shapes=json.loads((ROOT/'screen_manager/app/boards.json').read_text())
    fonts=[]; boards=[]
    chars=''.join(chr(i) for i in range(32,127))+'°·'
    icons=''.join(chr(i) for i in (0xF075A,0xF050F,0xF0335,0xF070D,0xF040A,0xF04AE,0xF04DB,0xF03E4,0xF02DC,0xF04AD,0xF0374,0xF0415,0xF05F8,0xF0425,0xF0717,0xF0238,0xF057E))
    for b in BOARDS:
        base=b.removesuffix('_portrait')
        s=profiles.substitutions(f'checkout/{base}.yaml');shape=shapes[f'checkout/{base}.yaml']; side=shape['orientations']['portrait' if b.endswith('_portrait') else 'landscape']
        entries=(('name','LABEL','Roboto-700.ttf'),('note','SUBLABEL','Roboto-400.ttf'),('headline','HEADLINE','Roboto-500.ttf'),('value','WATCH_VALUE','Roboto-500.ttf'),('digits','SETPOINT','Roboto-400.ttf'),('icon','ICON_MINI','materialdesignicons-webfont.ttf'))
        for role,key,file in entries:
            fonts.append(f'  - file: "{ROOT}/fonts/{file}"\n    id: {b}_{role}\n    size: {s["FONT_"+key+"_SIZE"]}\n    bpp: 4\n    glyphs: {json.dumps(icons if role=="icon" else chars, ensure_ascii=False)}\n')
        nums=[side['width'],side['height'],round(float(s['DISPLAY_DPI'])),side['columns'],side['rows'],*[int(s[k]) for k in ('SCROLL_Y','PAGE_BAR_H','GRID_MARGIN','GRID_GAP_X','GRID_GAP_Y')]]
        fs=', '.join(f'id({b}_{role})->get_lv_font()' for role,_,_ in entries)
        boards.append('{"'+b+'", "'+s['LOOK']+'", '+', '.join(map(str,nums))+', {'+fs+'}}')
    header=(ROOT/'tools/render/tall_concepts.h').read_text().replace('"../../components/',f'"{ROOT}/components/')
    (WORK/'concepts.h').write_text(header)
    lam='std::vector<tall_concepts::Board> boards = {'+', '.join(boards)+'};\n'
    lam+='for (auto &b : boards) for (int v=0;v<2;++v) {\n'
    for scene in SCENES:lam+=f'  tall_concepts::render(b,"{OUT}",v,"{scene}");\n'
    lam+=f'  tall_concepts::render(b,"{OUT}",v,"media",true);\n}}\nexit(0);'
    return f'''esphome:
  name: tall-concepts
  includes: ["{WORK}/concepts.h"]
  platformio_options:
    build_flags: [-DLV_USE_SNAPSHOT=1]
  on_boot:
    priority: -100
    then:
      - lambda: |-
{chr(10).join('          '+line for line in lam.splitlines())}
host:
logger:
  level: WARN
display:
  - platform: sdl
    id: glass
    dimensions:
      width: 1280
      height: 1280
    auto_clear_enabled: false
    update_interval: never
font:
{''.join(fonts)}
lvgl:
  default_font: guition_note
  displays: glass
  pages:
    - id: study
      pad_all: 0
      scrollable: false
      widgets:
        - label:
            hidden: true
            text: ""
'''

def sheets():
    for path in OUT.glob('*.ppm'):
        with Image.open(path) as im: im.save(path.with_suffix('.png'))
    font=ImageFont.truetype(str(ROOT/'fonts/Roboto-500.ttf'),22)
    small=ImageFont.truetype(str(ROOT/'fonts/Roboto-400.ttf'),17)
    for board in BOARDS:
        for scene in ('media','climate','tall'):
            pictures=[Image.open(OUT/f'{board}-{v}-{scene}.png').convert('RGB') for v in ('A','B')]
            # Native pixels at 1x or integer 2x for small screens; no redrawing of UI.
            scale=2 if pictures[0].width<=480 else 1
            w,h=pictures[0].size;canvas=Image.new('RGB',(2*w*scale+72,h*scale+112),'#f5f6f8');d=ImageDraw.Draw(canvas)
            d.text((24,16),f'{board} · {w} × {h} · {scene}',font=font,fill='#17202c')
            for i,im in enumerate(pictures):
                x=24+i*(w*scale+24);d.text((x,52),'A  Calm stack' if i==0 else 'B  Visual focus',font=small,fill='#56616d')
                canvas.paste(im.resize((w*scale,h*scale),Image.Resampling.NEAREST),(x,88))
            canvas.save(OUT/f'compare-{board}-{scene}.png')
    names={'cyd':'CYD · 2.8″ · 320 × 240','guition':'Guition · 4″ · 480 × 480','waveshare43':'Waveshare · 4.3″ · 800 × 480','waveshare7':'Waveshare · 7″ · 800 × 480','jc8012p4a1':'Guition P4 · 10.1″ · 1280 × 800','waveshare43_portrait':'Waveshare · portret · 480 × 800'}
    sections=[]
    for b in BOARDS:
        comparisons=''.join(f'<h3>{title}</h3><a href="compare-{b}-{scene}.png"><img src="compare-{b}-{scene}.png" alt="{title}: voorstel A links en B rechts" loading="lazy"></a>' for scene,title in [('media','Media'),('climate','Klimaat'),('tall','Smalle hoge tegels · 1×2')])
        baseline=''
        if b in ('cyd','guition','waveshare43') and (OUT.parents[1]/'baseline-focused'/b/'square-1.png').exists():
            baseline='<details><summary>Huidige firmware · nulmeting</summary><div class="extras">'+''.join(f'<figure><img src="../../baseline-focused/{b}/{file}.png" alt="{label}" loading="lazy"><figcaption>{label}</figcaption></figure>' for file,label in [('square-1','Huidige media 2×2'),('square-2','Huidige klimaat 2×2'),('tall-1','Huidige media 1×2'),('tall-2','Huidige klimaat 1×2')])+'</div></details>'
        extras='<details><summary>Andere gekozen bediening, licht, stofzuiger en donker</summary><div class="extras">'+''.join(f'<figure><img src="{b}-B-{scene}.png" alt="{label}" loading="lazy"><figcaption>{label}</figcaption></figure>' for scene,label in [('media-volume','Media · volume en dempen'),('media-none','Media · zonder bediening'),('climate-modes','Klimaat · uitsluitend modi'),('climate-none','Klimaat · zonder bediening'),('light','Licht · helderheid'),('vacuum','Stofzuiger · gekozen acties'),('media-dark','Media · donker')])+'</div></details>'
        sections.append(f'<section data-board="{b}"'+('' if b=='guition' else ' hidden')+f'><h2>{names[b]}</h2>'+('<p>Dit portretraster heeft één kolom. Alle voorstellen gebruiken hier 1×2.</p>' if b.endswith('_portrait') else '<p>Media en klimaat: 2×2. Daaronder: dezelfde kaarten naast elkaar als 1×2.</p>')+baseline+comparisons+extras+'</section>')
    options=''.join(f'<option value="{b}"'+(' selected' if b=='guition' else '')+f'>{names[b]}</option>' for b in BOARDS)
    html='''<!doctype html><html lang="nl"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Hoge tegels · LVGL ontwerpstudie</title><style>body{font:16px system-ui;margin:0;background:#f5f6f8;color:#17202c}main{max-width:1500px;margin:auto;padding:32px}h1{font-size:32px;margin-bottom:12px}p{max-width:900px;line-height:1.6;color:#56616d}img{max-width:100%;height:auto;display:block}section{margin:36px 0}h3{margin:36px 0 12px}select{font:inherit;padding:12px;border-radius:10px;border:1px solid #ccd2da;background:white;max-width:100%}.legend{display:flex;gap:24px;flex-wrap:wrap;margin:24px 0}.legend div{flex:1;min-width:220px;background:white;padding:20px;border-radius:16px}.extras{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:20px}figure{margin:0}figcaption{margin:10px 0 20px;color:#56616d}details{margin:24px 0;padding:16px;background:white;border-radius:12px}summary{cursor:pointer;margin-bottom:16px;font-weight:600}label{display:block;margin-bottom:8px}a{color:inherit}footer{color:#56616d;border-top:1px solid #dce1e8;padding:24px 0;font-size:14px}[hidden]{display:none}</style><main><h1>Meer hoogte, een andere indeling</h1><p>Echte LVGL-renders, met de fonts, rastermaten en pixeldichtheid van ieder scherm. Je gekozen bediening blijft gelijk. De ontwerpen voegen niet automatisch knoppen toe.</p><div class="legend"><div><strong>A · Rustige opbouw</strong><p>Informatie boven, bediening onder. Klimaat toont de gemeten temperatuur groot en het doel in de bedieningsbalk.</p></div><div><strong>B · Visuele nadruk</strong><p>Media krijgt albumbeeld waar dat past. Klimaat toont de doeltemperatuur groot; min en plus staan eronder. De gemeten temperatuur blijft als bijschrift zichtbaar als er ruimte is.</p></div></div><label for="board">Vergelijk op een scherm</label><select id="board">OPTIONS</select>SECTIONS<footer>Ontwerpstudie, nog geen nieuwe firmware. Albumbeeld is een illustratieve LVGL-placeholder. Bediening is hier alleen getekend, zonder Home Assistant-acties. De nulmeting gebruikt de bestaande firmware; de voorstellen gebruiken een afzonderlijke host-renderer.</footer></main><script>document.querySelector('#board').addEventListener('change',e=>document.querySelectorAll('[data-board]').forEach(s=>s.hidden=s.dataset.board!==e.target.value));</script></html>'''
    (OUT/'index.html').write_text(html.replace('OPTIONS',options).replace('SECTIONS',''.join(sections)))

def main():
    OUT.mkdir(parents=True,exist_ok=True);config=WORK/'tall-concepts.yaml';config.write_text(project())
    esphome=os.environ.get('ESPHOME',str(Path(sys.executable).with_name('esphome')))
    result=subprocess.run([esphome,'compile',str(config)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    (WORK/'build.log').write_text(result.stdout)
    if result.returncode: print(result.stdout[-7000:]);raise SystemExit(result.returncode)
    program=WORK/'.esphome/build/tall-concepts/.pioenvs/tall-concepts/program'
    env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_RENDER_DRIVER':'software'}
    result=subprocess.run([str(program)],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
    (WORK/'render.log').write_text(result.stdout)
    if result.returncode:print(result.stdout[-4000:]);raise SystemExit(result.returncode)
    sheets();print(OUT/'index.html')
if __name__=='__main__':main()

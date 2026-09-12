"""Guided USB pixel verification for GT911, without resistive calibration or HA actions."""
import argparse
import json
import math
from pathlib import Path
import re
import statistics
import time
from calibrate import write_file

TARGETS = [('linksboven', (24, 24)), ('rechtsboven', (455, 24)),
           ('rechtsonder', (455, 455)), ('linksonder', (24, 455)), ('midden', (240, 240))]
PATTERN = re.compile(r'GT911 press x=(\d+) y=(\d+) test=([01])')


def screen_point(x, y, rotation=0):
    if rotation == 0: return x, y
    if rotation == 90: return y, 479-x
    if rotation == 180: return 479-x, 479-y
    if rotation == 270: return 479-y, x
    raise ValueError('Rotatie moet 0, 90, 180 of 270 zijn.')


def verify(data):
    if data.get('screen') != [480,480] or len(data.get('points',[])) != 5:
        raise ValueError('Verwacht vijf meetpunten op een 480x480 GT911-scherm.')
    result=[]
    for p,(name,target) in zip(data['points'],TARGETS):
        if p.get('name') != name or len(p.get('samples',[])) != 3:
            raise ValueError('Volg de vijf doelen; ieder doel vereist drie aparte tikken.')
        samples=[]
        for xy in p['samples']:
            if len(xy)!=2 or any(type(v) is not int or not 0<=v<480 for v in xy):
                raise ValueError('Touchcoördinaten vallen buiten 480x480.')
            samples.append(screen_point(*xy, data.get('rotation',0)))
        center=tuple(statistics.median(x[i] for x in samples) for i in (0,1))
        error=math.dist(center,target)
        spread=max(math.dist(x,center) for x in samples)
        result.append(dict(name=name,error_px=round(error,1),spread_px=round(spread,1)))
        if error>16 or spread>18:
            raise ValueError(f'{name}: fout {error:.1f}px / spreiding {spread:.1f}px. Controleer rotatie en GT911-transform; geen ADC-fit gebruiken.')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', help='USB-poort; firmware moet het GT911-meetscherm tonen')
    parser.add_argument('--output',type=Path,default=Path('measurements-guition.json'))
    parser.add_argument('--input',type=Path,help='Controleer een bestaand meetbestand zonder USB')
    parser.add_argument('--rotation',type=int,choices=(0,90,180,270),default=0)
    args=parser.parse_args()
    try:
        if args.input:
            data=json.loads(args.input.read_text())
        else:
            import serial
            if not args.port: raise ValueError('--port is nodig voor een fysieke meting.')
            if args.output.exists(): raise ValueError('Meetbestand bestaat al; kies een andere naam.')
            data=dict(screen=[480,480],rotation=args.rotation,points=[])
            with serial.Serial(args.port,115200,timeout=.2) as port:
                for name,xy in TARGETS:
                    input(f'{name} {xy}: druk Enter en tik daarna drie keer alleen dit kruisje, met loslaten ertussen. ')
                    port.reset_input_buffer()
                    samples=[]; deadline=time.monotonic()+90; last=0
                    while len(samples)<3 and time.monotonic()<deadline:
                        m=PATTERN.search(port.readline().decode(errors='replace'))
                        if not m: continue
                        if m[3]!='1': raise ValueError('Het geïsoleerde GT911-meetscherm is niet actief.')
                        if time.monotonic()-last < .4:continue
                        samples.append([int(m[1]),int(m[2])]);last=time.monotonic()
                        print(f'  {len(samples)}/3',flush=True)
                    if len(samples)!=3:raise ValueError(f'{name}: onvoldoende tikken ontvangen.')
                    data['points'].append(dict(name=name,samples=samples))
            write_file(args.output,json.dumps(data,indent=2)+'\n')
        for row in verify(data):print(row)
        print('PASS: alle vijf GT911-doelen geverifieerd; geen HA-acties uitgevoerd.')
    except (ValueError,OSError,TypeError,KeyError,EOFError) as exc:
        parser.exit(1,f'Fout: {exc}\n')
    except KeyboardInterrupt:
        parser.exit(130,'Meting afgebroken.\n')


if __name__=='__main__': main()

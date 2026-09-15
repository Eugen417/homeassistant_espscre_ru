"""Guided USB pixel verification for GT911, without resistive calibration or HA actions."""
import argparse
import json
import math
from pathlib import Path
import re
import statistics
import time
from calibrate import write_file

TARGETS = [('top-left', (24, 24)), ('top-right', (455, 24)),
           ('bottom-right', (455, 455)), ('bottom-left', (24, 455)), ('center', (240, 240))]
PATTERN = re.compile(r'GT911 press x=(\d+) y=(\d+) test=([01])')


def screen_point(x, y, rotation=0):
    if rotation == 0: return x, y
    if rotation == 90: return y, 479-x
    if rotation == 180: return 479-x, 479-y
    if rotation == 270: return 479-y, x
    raise ValueError('Rotation must be 0, 90, 180, or 270.')


def verify(data):
    if data.get('screen') != [480,480] or len(data.get('points',[])) != 5:
        raise ValueError('Expected five measurement points on a 480x480 GT911 screen.')
    result=[]
    for p,(name,target) in zip(data['points'],TARGETS):
        if p.get('name') != name or len(p.get('samples',[])) != 3:
            raise ValueError('Follow the five targets; each target requires three separate taps.')
        samples=[]
        for xy in p['samples']:
            if len(xy)!=2 or any(type(v) is not int or not 0<=v<480 for v in xy):
                raise ValueError('Touch coordinates fall outside 480x480.')
            samples.append(screen_point(*xy, data.get('rotation',0)))
        center=tuple(statistics.median(x[i] for x in samples) for i in (0,1))
        error=math.dist(center,target)
        spread=max(math.dist(x,center) for x in samples)
        result.append(dict(name=name,error_px=round(error,1),spread_px=round(spread,1)))
        if error>16 or spread>18:
            raise ValueError(f'{name}: error {error:.1f}px / spread {spread:.1f}px. Check rotation and the GT911 transform; do not use an ADC fit.')
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', help='USB port; firmware must show the GT911 measurement screen')
    parser.add_argument('--output',type=Path,default=Path('measurements-guition.json'))
    parser.add_argument('--input',type=Path,help='Check an existing measurement file without USB')
    parser.add_argument('--rotation',type=int,choices=(0,90,180,270),default=0)
    args=parser.parse_args()
    try:
        if args.input:
            data=json.loads(args.input.read_text())
        else:
            import serial
            if not args.port: raise ValueError('--port is required for a physical measurement.')
            if args.output.exists(): raise ValueError('Measurement file already exists; choose a different name.')
            data=dict(screen=[480,480],rotation=args.rotation,points=[])
            with serial.Serial(args.port,115200,timeout=.2) as port:
                for name,xy in TARGETS:
                    input(f'{name} {xy}: press Enter, then tap only this crosshair three times, releasing in between. ')
                    port.reset_input_buffer()
                    samples=[]; deadline=time.monotonic()+90; last=0
                    while len(samples)<3 and time.monotonic()<deadline:
                        m=PATTERN.search(port.readline().decode(errors='replace'))
                        if not m: continue
                        if m[3]!='1': raise ValueError('The isolated GT911 measurement screen is not active.')
                        if time.monotonic()-last < .4:continue
                        samples.append([int(m[1]),int(m[2])]);last=time.monotonic()
                        print(f'  {len(samples)}/3',flush=True)
                    if len(samples)!=3:raise ValueError(f'{name}: not enough taps received.')
                    data['points'].append(dict(name=name,samples=samples))
            write_file(args.output,json.dumps(data,indent=2)+'\n')
        for row in verify(data):print(row)
        print('PASS: all five GT911 targets verified; no HA actions performed.')
    except (ValueError,OSError,TypeError,KeyError,EOFError) as exc:
        parser.exit(1,f'Error: {exc}\n')
    except KeyboardInterrupt:
        parser.exit(130,'Measurement aborted.\n')


if __name__=='__main__': main()

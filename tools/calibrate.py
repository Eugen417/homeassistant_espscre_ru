"""USB calibration: guided capture, affine fit with center hold-out, verification.

No network or Home Assistant access. The device must already run the firmware
with CALIBRATION_ON_BOOT=true (or have its isolated diagnostic page opened).
"""
import argparse
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import re
import statistics
import time

TARGETS = [('top-left', (20, 20)), ('top-right', (299, 20)),
           ('bottom-right', (299, 219)), ('bottom-left', (20, 219)), ('center', (160, 120))]
BOUNDS = (280, 3860, 340, 3860)
PATTERN = re.compile(r'press native=(\d+),(\d+) raw=(\d+),(\d+) pressure=(\d+) calibration=([01])')


def parse_touch(line):
    match = PATTERN.search(line)
    if not match:
        return None
    nx, ny, rx, ry, pressure, isolated = map(int, match.groups())
    if not isolated:
        raise ValueError('Not an isolated measurement screen: start with CALIBRATION_ON_BOOT=true. No data saved.')
    return dict(native_x=nx, native_y=ny, raw_x=rx, raw_y=ry, pressure=pressure)


def write_file(path, contents, replace=False):
    path = Path(path)
    if path.exists():
        if not replace:
            raise ValueError(f'{path} already exists; choose a different name or pass --replace explicitly.')
        backup = path.with_name(path.name + '.bak-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
        backup.write_bytes(path.read_bytes())
    # Write completely before replacing an existing configuration.
    temporary = path.with_name(path.name + '.tmp-' + str(os.getpid()))
    try:
        with temporary.open('x', encoding='utf-8') as handle:
            handle.write(contents)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def validate(data):
    if data.get('schema') != 1 or data.get('screen') != [320, 240]:
        raise ValueError('Expected schema 1 and landscape 320×240; other orientations are not supported.')
    points = data.get('points', [])
    if len(points) != len(TARGETS):
        raise ValueError('All four corners and the independent center point are required.')
    for point, (name, xy) in zip(points, TARGETS):
        if point.get('name') != name or point.get('screen') != list(xy):
            raise ValueError("Measurement points/order don't match the wizard.")
        if len(point.get('samples', [])) < 3:
            raise ValueError(f'{name}: at least three separate taps required.')
        for sample in point['samples']:
            for key, maximum in [('raw_x', 4095), ('raw_y', 4095), ('native_x', 239), ('native_y', 319), ('pressure', 8190)]:
                value = sample.get(key)
                if type(value) is not int or not 0 <= value <= maximum:
                    raise ValueError(f'{name}: invalid {key}.')
    return points


def median_raw(point):
    return tuple(statistics.median(s[key] for s in point['samples']) for key in ('raw_x', 'raw_y'))


def least_squares(rows, values):
    matrix = [[sum(row[i] * row[j] for row in rows) for j in range(3)] +
              [sum(row[i] * value for row, value in zip(rows, values))] for i in range(3)]
    for column in range(3):
        pivot = max(range(column, 3), key=lambda row: abs(matrix[row][column]))
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        scale = matrix[column][column]
        if abs(scale) < 1e-9:
            raise ValueError('Measurement points are not independent; likely the same corner was measured more than once.')
        matrix[column] = [v / scale for v in matrix[column]]
        for row in range(3):
            if row != column:
                factor = matrix[row][column]
                matrix[row] = [a - factor * b for a, b in zip(matrix[row], matrix[column])]
    return [matrix[i][3] for i in range(3)]


def project(coefficients, rx, ry):
    xx, xy, xc, yx, yy, yc = coefficients
    xmin, xmax, ymin, ymax = BOUNDS
    return ((yx * rx + yy * ry + yc - ymin) * 320 / (ymax - ymin),
            (xx * rx + xy * ry + xc - xmin) * 240 / (xmax - xmin))


def fit(data, tolerance=12):
    points = validate(data)
    medians = [median_raw(p) for p in points]
    # Scale inputs to avoid poorly conditioned normal equations on 12-bit ADC data.
    rows = [(x / 4095, y / 4095, 1) for x, y in medians[:4]]
    xmin, xmax, ymin, ymax = BOUNDS
    values_x = [xmin + p['screen'][1] * (xmax - xmin) / 240 for p in points[:4]]
    values_y = [ymin + p['screen'][0] * (ymax - ymin) / 320 for p in points[:4]]
    cx, cy = least_squares(rows, values_x), least_squares(rows, values_y)
    coefficients = [cx[0] / 4095, cx[1] / 4095, cx[2], cy[0] / 4095, cy[1] / 4095, cy[2]]
    if not all(math.isfinite(v) for v in coefficients):
        raise ValueError('Invalid calibration coefficients.')
    # Reject captures that amplify small sensor jitter into huge screen movements.
    if max(abs(v) for v in (coefficients[0], coefficients[1], coefficients[3], coefficients[4])) > 4:
        raise ValueError('Measurement range too small; tap the actual screen corners.')
    report = []
    for point, median in zip(points, medians):
        predicted = project(coefficients, *median)
        error = math.dist(predicted, point['screen'])
        spread = max(math.dist(project(coefficients, s['raw_x'], s['raw_y']), predicted) for s in point['samples'])
        report.append(dict(name=point['name'], median=list(median), error_px=round(error, 2), spread_px=round(spread, 2)))
        if error > tolerance or spread > 18:
            raise ValueError(f"{point['name']}: error {error:.1f}px, spread {spread:.1f}px. "
                             "Measure again; don't calibrate away an unstable panel.")
    return coefficients, report


def calibration_yaml(coefficients):
    names = ['TOUCH_AFFINE_XX', 'TOUCH_AFFINE_XY', 'TOUCH_AFFINE_XC', 'TOUCH_AFFINE_YX', 'TOUCH_AFFINE_YY', 'TOUCH_AFFINE_YC']
    lines = ['# Measured with tools/calibrate.py; only valid for this physical panel.', 'substitutions:']
    for name, value in zip(['TOUCH_CAL_X_MIN', 'TOUCH_CAL_X_MAX', 'TOUCH_CAL_Y_MIN', 'TOUCH_CAL_Y_MAX'], BOUNDS):
        lines.append(f'  {name}: "{value}"')
    lines += [f'  {name}: "{value:.9f}"' for name, value in zip(names, coefficients)]
    return '\n'.join(lines) + '\n'


def verify(data, tolerance=12):
    results = []
    for point in validate(data):
        measured = [(s['native_y'], 239 - s['native_x']) for s in point['samples']]
        center = tuple(statistics.median(p[i] for p in measured) for i in (0, 1))
        error = math.dist(center, point['screen'])
        spread = max(math.dist(p, center) for p in measured)
        results.append(dict(name=point['name'], error_px=round(error, 2), spread_px=round(spread, 2)))
        if error > tolerance or spread > 18:
            raise ValueError(f"{point['name']}: real firmware coordinates deviate {error:.1f}px (spread {spread:.1f}px).")
    return results


def capture(args):
    import serial
    if args.output.exists():
        raise ValueError('Measurement file already exists; choose a new name.')
    data = dict(schema=1, device_name=args.device_name, screen=[320, 240],
                created_at=datetime.now(timezone.utc).isoformat(), points=[])
    print('Only for 320×240 / LVGL 90° / swap_xy=false / mirror_x=true / mirror_y=false.')
    print('The screen must show the FIVE crosshairs. Close other serial log readers.')
    with serial.Serial(args.port, 115200, timeout=0.2) as port:
        for name, xy in TARGETS:
            input(f'\nNext point: {name} {xy}. Press ENTER, then tap only this crosshair {args.samples} times: ')
            port.reset_input_buffer()
            samples = []
            deadline = time.monotonic() + args.timeout
            last = -math.inf
            while len(samples) < args.samples and time.monotonic() < deadline:
                sample = parse_touch(port.readline().decode('utf-8', errors='replace'))
                if sample and time.monotonic() - last >= 0.4:
                    samples.append(sample)
                    last = time.monotonic()
                    print(f'  {name}: {len(samples)}/{args.samples}', flush=True)
            if len(samples) != args.samples:
                raise ValueError(f'{name}: not enough valid taps. Check the firmware, the measurement screen, and the USB port.')
            data['points'].append(dict(name=name, screen=list(xy), samples=samples))
    write_file(args.output, json.dumps(data, indent=2) + '\n')
    print(f'Measurement saved: {args.output}. The script did not flash or control anything.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    c = commands.add_parser('capture', help='Read guided taps over USB; firmware must already show the measurement screen.')
    c.add_argument('--port', required=True)
    c.add_argument('--output', type=Path, required=True)
    c.add_argument('--device-name', default='unknown')
    c.add_argument('--samples', type=int, choices=range(3, 10), default=3)
    c.add_argument('--timeout', type=float, default=60)
    for name in ('fit', 'verify'):
        sub = commands.add_parser(name)
        sub.add_argument('input', type=Path)
        sub.add_argument('--tolerance', type=float, default=12)
        if name == 'fit':
            sub.add_argument('--output', type=Path, default=Path('calibration.proposed.yaml'))
            sub.add_argument('--replace', action='store_true')
    args = parser.parse_args()
    try:
        if args.command == 'capture':
            if args.timeout <= 0: raise ValueError('Timeout must be positive.')
            capture(args)
        else:
            if args.tolerance <= 0 or not math.isfinite(args.tolerance): raise ValueError('Tolerance must be finite and positive.')
            data = json.loads(args.input.read_text())
            if args.command == 'fit':
                coefficients, report = fit(data, args.tolerance)
                write_file(args.output, calibration_yaml(coefficients), args.replace)
                print(f'Calibration written: {args.output}. The center point was independently checked.')
            else:
                report = verify(data, args.tolerance)
                print('PASS: independent physical check of the flashed coordinates.')
            for item in report:
                print(f"{item['name']}: error={item['error_px']}px, spread={item['spread_px']}px")
    except (ValueError, OSError, KeyError, TypeError, EOFError) as exc:
        parser.exit(1, f'Error: {exc}\n')
    except KeyboardInterrupt:
        parser.exit(130, 'Aborted; no partial measurement saved.\n')


if __name__ == '__main__':
    main()

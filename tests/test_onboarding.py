"""Offline regression tests for installation, calibration and safe export."""
import copy
from pathlib import Path
import re
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import calibrate
import new_device
import export_bundle


def measurements():
    points = []
    for name, (x, y) in calibrate.TARGETS:
        rx = round(280 + y * 3580 / 240)
        ry = round(340 + x * 3520 / 320)
        samples = [dict(raw_x=rx+d, raw_y=ry+d, native_x=239-y,
                        native_y=x, pressure=1500) for d in (-2, 0, 2)]
        points.append(dict(name=name, screen=[x, y], samples=samples))
    return dict(schema=1, screen=[320, 240], points=points)


class CalibrationTests(unittest.TestCase):
    def test_identity_fit_and_independent_verification(self):
        coefficients, report = calibrate.fit(measurements())
        self.assertAlmostEqual(coefficients[0], 1, places=3)
        self.assertAlmostEqual(coefficients[4], 1, places=3)
        self.assertTrue(all(p['error_px'] < 1 for p in report))
        self.assertEqual(len(calibrate.verify(measurements())), 5)

    def test_affine_skew_is_corrected(self):
        data = measurements()
        for point in data['points']:
            for s in point['samples']:
                x, y = s['raw_x'], s['raw_y']
                s['raw_x'] = round(.9*x - .03*y + 190)
                s['raw_y'] = round(.04*x + .95*y + 50)
        _, report = calibrate.fit(data)
        self.assertTrue(all(p['error_px'] < 1 for p in report))

    def test_center_is_not_used_to_hide_bad_fit(self):
        data = measurements()
        for s in data['points'][4]['samples']: s['raw_x'] += 600
        with self.assertRaisesRegex(ValueError, 'midden'): calibrate.fit(data)

    def test_jitter_rejected(self):
        data = measurements()
        data['points'][0]['samples'][0]['raw_x'] += 700
        with self.assertRaises(ValueError): calibrate.fit(data)

    def test_repeated_corner_rejected(self):
        data = measurements()
        for p in data['points']: p['samples'] = copy.deepcopy(data['points'][0]['samples'])
        with self.assertRaises(ValueError): calibrate.fit(data)

    def test_wrong_firmware_coordinates_rejected(self):
        data = measurements()
        for s in data['points'][0]['samples']: s['native_y'] += 40
        with self.assertRaises(ValueError): calibrate.verify(data)

    def test_capture_requires_isolated_mode(self):
        line = 'press native=20,30 raw=500,600 pressure=1500 calibration='
        self.assertEqual(calibrate.parse_touch(line+'1')['raw_x'], 500)
        with self.assertRaises(ValueError): calibrate.parse_touch(line+'0')
        self.assertIsNone(calibrate.parse_touch('unrelated log'))

    def test_wrong_screen_or_order_rejected(self):
        for field in ('screen', 'order'):
            data = measurements()
            if field == 'screen': data['screen'] = [240,320]
            else: data['points'].reverse()
            with self.assertRaises(ValueError): calibrate.fit(data)

    def test_output_requires_explicit_replace_and_preserves_backup(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'calibration.yaml'
            calibrate.write_file(path, 'old')
            with self.assertRaises(ValueError): calibrate.write_file(path, 'new')
            calibrate.write_file(path, 'new', replace=True)
            self.assertEqual(path.read_text(), 'new')
            self.assertEqual(next(Path(tmp).glob('*.bak-*')).read_text(), 'old')


class InstallationTests(unittest.TestCase):
    def test_unique_secrets_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
            new_device.create(a, 'display-a', 'Display A')
            new_device.create(b, 'display-b', 'Display B')
            original = (Path(a)/'secrets.yaml').read_text()
            self.assertNotEqual(original, (Path(b)/'secrets.yaml').read_text())
            self.assertNotIn('WORDT_UNIEK_GEGENEREERD', original)
            with self.assertRaises(ValueError): new_device.create(a, 'new', 'New')
            self.assertEqual(original, (Path(a)/'secrets.yaml').read_text())

    def test_invalid_device_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            for name in ('../oops', 'UPPER', 'has space', 'trailing-'):
                with self.assertRaises(ValueError): new_device.create(tmp, name, 'Test')
            self.assertEqual(list(Path(tmp).iterdir()), [])

    def test_export_is_neutral_and_has_required_sources(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp)/'starter.zip'
            export_bundle.export(ROOT, output)
            with zipfile.ZipFile(output) as archive:
                names = archive.namelist()
                for forbidden in ('secrets.yaml', 'device.yaml', 'calibration.yaml', '.git/', '.esphome/', '.log', '.bin', '.json'):
                    self.assertFalse(any(('/'+forbidden in n if forbidden.endswith('/') else n.endswith(forbidden) if forbidden.startswith('.') else n.endswith('/'+forbidden)) for n in names), forbidden)
                self.assertIn('cyd-display/.gitignore', names)
                base = archive.read('cyd-display/home-like-2432s028.yaml').decode()
                values = dict(re.findall(r'^  (\w+): "([^"]*)"', base, re.M))
                self.assertEqual(values['TOUCH_AFFINE_XX'], '1.000000000')
                self.assertEqual(values['DIRECT_ACTIONS'], 'false')
                for i in range(1,11):
                    self.assertRegex(values[f'TILE{i}_ENTITY'], r'\.(voorbeeld_|ongebruikt_)')
                archive.extractall(tmp)
            new_device.create(Path(tmp)/'cyd-display', 'fresh-board', 'Fresh board')
            with self.assertRaises(FileExistsError): export_bundle.export(ROOT, output)


if __name__ == '__main__': unittest.main()

import { expect, it } from 'vitest';
import { availableControl, controlKeys } from '../src/model/tall-controls';

it('shows exactly the supported media subset within the chosen group', () => {
  for (let flags = 0; flags < 64; flags++) {
    const a = { supported_features: flags };
    expect(controlKeys('media_player', 'playback', 'playing', a).map(k => k.icon)).toEqual([
      ...(flags & 16 ? ['skip-previous'] : []), ...(flags & 1 ? ['pause'] : []), ...(flags & 32 ? ['skip-next'] : []),
    ]);
    expect(Boolean(availableControl('media_player', 'volume', 'playing', a))).toBe(Boolean(flags & 12));
  }
  expect(controlKeys('media_player', 'playback', 'idle', { supported_features: 16384 })).toEqual([{ icon: 'play', disabled: false, primary: true }]);
});
it('uses cover capabilities, direction and end stops and vacuum capabilities/state', () => {
  expect(controlKeys('cover', 'buttons', 'open', { supported_features: 3, current_position: 100, device_class: 'curtain' })).toEqual([
    { icon: 'arrow-expand-horizontal', disabled: true, primary: false }, { icon: 'arrow-collapse-horizontal', disabled: false, primary: false },
  ]);
  expect(controlKeys('cover', 'buttons', 'closed', { supported_features: 0 })).toEqual([]);
  expect(controlKeys('vacuum', 'buttons', 'cleaning', { supported_features: 4 | 16 }).map(k => k.icon)).toEqual(['pause', 'home-map-marker']);
  expect(controlKeys('vacuum', 'buttons', 'docked', { supported_features: 8192 }).map(k => k.icon)).toEqual(['play']);
  expect(controlKeys('timer', 'buttons', 'active', {}).map(k => k.icon)).toEqual(['pause', 'close']);
});
it('keeps unsupported saved slider/setpoint choices inert', () => {
  for (const [domain, kind, attrs] of [
    ['light', 'brightness', { supported_color_modes: ['onoff'] }], ['fan', 'speed', { supported_features: 0 }],
    ['cover', 'position', { supported_features: 3 }], ['climate', 'setpoint', { supported_features: 2 }],
  ] as const) expect(availableControl(domain, kind, 'on', attrs)).toBe('');
  expect(availableControl('climate', 'setpoint', 'heat', { supported_features: 1 })).toBe('setpoint');
  expect(availableControl('light', 'brightness', 'on', { supported_color_modes: ['rgbww'] })).toBe('brightness');
  expect(availableControl('media_player', 'playback', 'unavailable', { supported_features: 49 })).toBe('');
  expect(availableControl('light', null, 'on', { supported_color_modes: ['brightness'] })).toBe('');
});
it('limits modes to real choices and disables selects with fewer than two options', () => {
  expect(controlKeys('climate', 'mode', 'cool', { hvac_modes: ['fan_only', 'cool', 'off'] }).map(k => k.mode)).toEqual(['fan_only', 'cool', 'off']);
  const six = { hvac_modes: ['off', 'heat_cool', 'cool', 'heat', 'fan_only', 'dry'] };
  expect(controlKeys('climate', 'mode', 'cool', six).map(k => k.mode ?? k.icon)).toEqual(['off', 'heat_cool', 'dots-horizontal']);
  expect(controlKeys('climate', 'mode', 'cool', six, 6).map(k => k.mode)).toEqual(six.hvac_modes);
  expect(controlKeys('select', 'stepper', 'Eco', { options: ['Eco'] }).every(k => k.disabled)).toBe(true);
});

it('keeps slats independent of the primary choice and follows every cover feature mask', async () => {
  const { coverPrimary, hasCoverTilt, withCoverTilt, coverTiltKind, coverTiltKeys } = await import('../src/model/tall-controls');
  for (const primary of ['none', 'buttons', 'position']) {
    const selected = withCoverTilt(primary, true);
    expect(hasCoverTilt(selected)).toBe(true);
    expect(coverPrimary(selected)).toBe(primary);
    expect(withCoverTilt(coverPrimary(selected), false)).toBe(primary);
  }
  for (let flags = 0; flags < 256; flags++) {
    const attrs = { supported_features: flags, current_tilt_position: 100 };
    expect(coverTiltKind('open', attrs)).toBe(flags & 128 ? 'position' : flags & 112 ? 'buttons' : '');
    expect(coverTiltKind('unavailable', attrs)).toBe('');
    expect(coverTiltKeys(attrs).map(k => k.icon)).toEqual([
      ...(flags & 16 ? ['blinds-open'] : []), ...(flags & 64 ? ['stop'] : []), ...(flags & 32 ? ['blinds'] : []),
    ]);
    expect(availableControl('cover', 'position_tilt', 'open', attrs)).toBe(flags & 4 ? 'position' : '');
    expect(availableControl('cover', 'tilt', 'open', attrs)).toBe('');
  }
});

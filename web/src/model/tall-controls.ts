// Preview counterpart of tile_controls::keys_for/panel_available. These are
// display descriptions only; the firmware owns touch and Home Assistant actions.
type Attributes = Record<string, any>;
export type ControlKey = { icon: string; primary?: boolean; disabled?: boolean; mode?: string };
export function controlKeys(domain: string, kind: string, state: string, a: Attributes): ControlKey[] {
  const f = Number(a.supported_features || 0), keys: ControlKey[] = [];
  const add = (icon: string, disabled = false, primary = false) => keys.push({ icon, disabled, primary });
  if (kind === 'playback') {
    if (f & 16) add('skip-previous');
    if (f & (1 | 16384)) add(state === 'playing' ? 'pause' : 'play', !(f & (state === 'playing' ? 1 : 16384)), true);
    if (f & 32) add('skip-next');
  } else if (kind === 'buttons' && domain === 'cover') {
    const sideways = ['curtain', 'awning', 'door', 'gate'].includes(a.device_class);
    const open = typeof a.current_position === 'number' ? a.current_position >= 99.5 : state === 'open';
    const closed = typeof a.current_position === 'number' ? a.current_position <= .5 : state === 'closed';
    if (f & 1) add(sideways ? 'arrow-expand-horizontal' : 'arrow-up', open && state !== 'closing');
    if (f & 8) add('stop');
    if (f & 2) add(sideways ? 'arrow-collapse-horizontal' : 'arrow-down', closed && state !== 'opening');
  } else if (kind === 'buttons' && domain === 'vacuum') {
    if (state === 'cleaning' && (f & 4 || f & 2 && !(f & 8192))) add('pause');
    else if (f & (8192 | 1)) add('play');
    if (f & 8) add('stop');
    if (f & 16) add('home-map-marker');
  } else if (kind === 'buttons' && domain === 'timer') {
    add(state === 'active' ? 'pause' : 'play'); add('close');
  } else if (kind === 'stepper' && domain.endsWith('select')) {
    add('chevron-left', (a.options?.length || 0) < 2); add('chevron-right', (a.options?.length || 0) < 2);
  } else if (kind === 'mode') {
    const icons: Record<string, string> = { off: 'power', heat: 'fire', cool: 'snowflake', heat_cool: 'sun-snowflake-variant', auto: 'thermostat-auto', dry: 'water-percent', fan_only: 'fan' };
    for (const [mode, icon] of Object.entries(icons))
      if (Array.isArray(a.hvac_modes) && a.hvac_modes.includes(mode) && keys.length < 3) keys.push({ icon, mode });
  }
  return keys;
}
export function availableControl(domain: string, kind: string | null, state: string, a: Attributes): string {
  if (!kind || ['unavailable', 'unknown', ''].includes(state)) return '';
  if (domain === 'cover') kind = coverPrimary(kind);
  const f = Number(a.supported_features || 0);
  if (['buttons', 'playback', 'mode'].includes(kind) || kind === 'stepper' && domain.endsWith('select'))
    return controlKeys(domain, kind, state, a).length ? kind : '';
  if (kind === 'brightness') return domain === 'light' && (!a.supported_color_modes?.length || a.supported_color_modes.some((mode: string) => ['brightness', 'white', 'color_temp', 'hs', 'rgb', 'rgbw', 'rgbww', 'xy'].includes(mode))) ? kind : '';
  if (kind === 'speed') return domain === 'fan' && f & 1 ? kind : '';
  if (kind === 'position') return domain === 'cover' && f & 4 ? kind : '';
  if (kind === 'volume') return domain === 'media_player' && f & 12 ? kind : '';
  if (kind === 'setpoint' || kind === 'setpoint_mode') return domain === 'climate' && f & 1 ? kind : '';
  if (kind === 'stepper' || kind === 'slider') return ['number', 'input_number'].includes(domain) ? kind : '';
  if (kind === 'toggle') return ['light', 'switch', 'input_boolean', 'fan'].includes(domain) ? kind : '';
  if (kind === 'run') return ['scene', 'script', 'button', 'input_button'].includes(domain) ? kind : '';
  return '';
}

// Preserve the primary choice while toggling the additional slat group.
export const hasCoverTilt = (kind: string | null | undefined) => ['tilt', 'buttons_tilt', 'position_tilt'].includes(kind || '');
export const coverPrimary = (kind: string | null | undefined) => kind === 'tilt' ? 'none' : (kind || 'none').replace(/_tilt$/, '');
export const withCoverTilt = (primary: string, tilt: boolean) => tilt ? (primary === 'none' ? 'tilt' : primary + '_tilt') : primary;
export function coverTiltKind(state: string, a: Attributes): '' | 'position' | 'buttons' {
  if (['unavailable', 'unknown', ''].includes(state)) return '';
  const f = Number(a.supported_features || 0);
  return f & 128 ? 'position' : f & 112 ? 'buttons' : '';
}
export function coverTiltKeys(a: Attributes): ControlKey[] {
  const f = Number(a.supported_features || 0), value = a.current_tilt_position;
  return [
    ...(f & 16 ? [{ icon: 'blinds-open', disabled: typeof value === 'number' && value >= 99.5 }] : []),
    ...(f & 64 ? [{ icon: 'stop' }] : []),
    ...(f & 32 ? [{ icon: 'blinds', disabled: typeof value === 'number' && value <= .5 }] : []),
  ];
}

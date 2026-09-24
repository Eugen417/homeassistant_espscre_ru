import { expect, it } from 'vitest';
import { suggestedPageTitle } from '../src/model/page-naming';
import { arrangeTiles, emptyLayout } from '../src/model/pages';

const page = (...ids: string[]) => arrangeTiles(emptyLayout('Home'), { columns: 2, rows: 3 },
  ids.map((entity, slot) => ({ tile: { entity, slot, name: '' }, slot }))).pages[0];
it('prefers the shared HA room over the entity domain', () => {
  expect(suggestedPageTitle(page('light.a', 'sensor.b'), [
    { id: 'light.a', name: 'Lamp', area: 'Study' }, { id: 'sensor.b', name: 'Temperature', area: 'Study' },
  ])).toBe('Study');
});
it('uses a domain for different or missing rooms and leaves mixed unknown content unnamed', () => {
  expect(suggestedPageTitle(page('light.a', 'light.b'), [])).toBe('Light');
  expect(suggestedPageTitle(page('light.a', 'sensor.b'), [])).toBeUndefined();
  expect(suggestedPageTitle(page('screen.clock'), [])).toBeUndefined();
});
it('bounds a long multibyte HA room name without corrupting its characters', () => {
  const title = suggestedPageTitle(page('light.a'), [{ id: 'light.a', name: 'Lamp', area: '寝'.repeat(100) }])!;
  expect(new TextEncoder().encode(title).length).toBe(96);
  expect(title).toBe('寝'.repeat(32));
});

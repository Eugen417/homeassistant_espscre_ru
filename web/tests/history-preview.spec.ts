import { describe, expect, it } from 'vitest';
import { historyGeometry } from '../src/model/history-preview';
describe('recorder graph geometry', () => {
  it('keeps missing buckets as gaps and zero as a real value', () => {
    const result = historyGeometry({ start: 0, end: 400, values: [0, 2, null, 4], unit: '°C' })!;
    expect(result.paths).toHaveLength(2);
    expect(result.paths[0]).toContain('M27.25,47');
    expect(result.paths[1]).toContain('M172.75,3');
    expect(historyGeometry({ start: 0, end: 1, values: [null, null], unit: '' })).toBeNull();
  });
  it('retains the actual time and magnitude of extrema alongside the averages', () => {
    const result = historyGeometry({ start: 100, end: 500, values: [2, 2, 2, 2], hi: [8, 400], lo: [0, 200], unit: '' })!;
    expect(result.points).toEqual([{ x: 148.5, y: 3 }, { x: 51.5, y: 47 }]);
  });
});

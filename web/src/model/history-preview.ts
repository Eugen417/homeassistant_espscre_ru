import { getJson } from '../api';
export type HistoryPreview = { start: number; end: number; values: (number | null)[]; dom?: [number, number]; hi?: [number, number]; lo?: [number, number]; unit: string };
const cache = new Map<string, { time: number; result: Promise<HistoryPreview | null> }>();
/** A bounded cache shared by map, focused page and navigation preview. */
export function loadHistory(entity: string, hours: number) {
  const key = `${entity}:${hours}`, previous = cache.get(key);
  if (previous && Date.now() - previous.time < 60000) return previous.result;
  const result = getJson<{ history: HistoryPreview | null }>(`history-preview?entity=${encodeURIComponent(entity)}&hours=${hours}`).then((data) => data.history);
  cache.set(key, { time: Date.now(), result });
  while (cache.size > 64) cache.delete(cache.keys().next().value!);
  result.catch(() => { if (cache.get(key)?.result === result) cache.delete(key); });
  return result;
}
/** Bucket centers retain their time positions; missing buckets break the line.
 * The recorder's extrema are drawn separately, so the average cannot hide them.
 */
export function historyGeometry(data: HistoryPreview) {
  const known = data.values.filter((value): value is number => value !== null && Number.isFinite(value));
  if (!known.length || data.end <= data.start) return null;
  const extrema = [data.hi, data.lo].filter((value): value is [number, number] => !!value && value.every(Number.isFinite));
  const [low, high] = data.dom || [Math.min(...known, ...extrema.map(([v]) => v)), Math.max(...known, ...extrema.map(([v]) => v))];
  const y = (value: number) => high === low ? 25 : 47 - (value - low) / (high - low) * 44;
  const x = (time: number) => 3 + (time - data.start) / (data.end - data.start) * 194;
  const paths: string[] = []; let segment: string[] = [];
  data.values.forEach((value, index) => {
    if (value === null || !Number.isFinite(value)) { if (segment.length) paths.push(segment.join(' ')); segment = []; return; }
    segment.push(`${segment.length ? 'L' : 'M'}${x(data.start + (index + .5) * (data.end - data.start) / data.values.length)},${y(value)}`);
  });
  if (segment.length) paths.push(segment.join(' '));
  return { paths, points: extrema.filter(([, time]) => time >= data.start && time <= data.end).map(([value, time]) => ({ x: x(time), y: y(value) })) };
}

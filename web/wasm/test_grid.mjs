// The grid a layout brings (firmware 0.53.0+): the real receiver changes the screen's grid with begin, makes the cards
// of the new grid, places the tiles on it, and refuses a grid it does not take without changing anything.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import createModule from '../src/wasm/firmware_preview.js';

const wasmBinary = readFileSync(new URL('../src/wasm/firmware_preview.wasm', import.meta.url));
const m = await createModule({ wasmBinary });
assert.equal(m._preview_init(480, 480, 170, 2, 3), 1);
let ms = 0;
const tick = (delta = 32) => { ms += delta; m._preview_time(ms, 1789401840, 7200); m._preview_render(); };
const raw = (packet) => m.ccall('preview_receive', 'string', ['string'], [JSON.stringify(packet)]);
let seq = 0, revision = 0, granted = '';
const send = (packet, rev) => raw({ ...packet, v: 2, session: granted.slice(8), seq: ++seq, rev });
const layout = () => JSON.parse(m.ccall('preview_layout', 'string', [], []));

// Every new layout starts with a hello, as ESP Screens sends it (page_delivery.Sender.synchronize).
function deliver(grid, tiles) {
  granted = raw({ v: 2, op: 'hello', request: (revision + 1).toString(16).padStart(16, '0') });
  assert.match(granted, /^Session:[0-9a-f]{16}$/);
  seq = 0;
  const rev = (++revision).toString(16).padStart(16, '0');
  const begin = send({ op: 'begin', title: 'Grid', pages: 1, tiles: tiles.length, home: 0, keepalive: 60, ...(grid ? { grid } : {}) }, rev);
  if (begin.startsWith('Error')) return begin;
  const page = send({ op: 'page', p: 0, id: '0000000000000001', title: '', home_control: false, excluded: false, items: [] }, rev);
  const answers = tiles.map((slot, i) => send({ op: 'tile', i, slot, entity: `light.l${i}`, name: `L${i}`, state: 'on', a: { brightness: 200 }, o: {} }, rev));
  assert.ok([page, ...answers].every((answer) => answer === 'Loading tiles'), [page, ...answers].join(', '));
  const commit = send({ op: 'commit' }, rev);
  for (let i = 0; i < 40; ++i) tick(50);
  return commit;
}
const placed = () => {
  const now = layout();
  const boxes = now.cards.map((card) => now.objects[card.object])
    .map((box) => ({ x: box.x1, y: box.y1, width: box.x2 - box.x1 + 1, height: box.y2 - box.y1 + 1 }));
  return { columns: now.columns, rows: now.rows, boxes };
};

// The board's own grid first, then one with a row more: eight cells, every tile in its own.
assert.equal(deliver(null, [0, 1, 2, 3, 4, 5]), 'Synced');
let now = placed();
assert.deepEqual([now.columns, now.rows], [2, 3]);
assert.equal(now.boxes.length, 6);
assert.equal(deliver([2, 4], [0, 1, 2, 3, 4, 5, 6, 7]), 'Synced');
now = placed();
assert.deepEqual([now.columns, now.rows], [2, 4]);
assert.equal(now.boxes.length, 8, 'a card for every cell of the new grid');
const heights = new Set(now.boxes.map((box) => box.height));
assert.equal(heights.size, 1, 'every cell of a row as high as the next');
const rows = [...new Set(now.boxes.map((box) => box.y))].sort((a, b) => a - b);
assert.equal(rows.length, 4, 'four rows of cards');
for (let i = 1; i < rows.length; ++i) assert.ok(rows[i] >= rows[i - 1] + now.boxes[0].height, 'rows do not overlap');

// Three columns of two: the cards are made again, narrower.
assert.equal(deliver([3, 2], [0, 1, 2, 3, 4, 5]), 'Synced');
now = placed();
assert.deepEqual([now.columns, now.rows], [3, 2]);
assert.equal(now.boxes.length, 6);
assert.equal(new Set(now.boxes.map((box) => box.y)).size, 2);
assert.equal(new Set(now.boxes.map((box) => box.x)).size, 3);

// A grid past what the screen takes is refused whole: the grid and the layout stay as they were.
assert.equal(deliver([9, 9], [0]), 'Error: grid');
assert.equal(deliver([0, 3], [0]), 'Error: grid');
now = placed();
assert.deepEqual([now.columns, now.rows], [3, 2]);
assert.equal(now.boxes.length, 6);

// The same grid again is no change: the cards stay, the layout comes through.
assert.equal(deliver([3, 2], [0, 1]), 'Synced');
assert.equal(placed().boxes.length, 2);
console.log('PASS grids: begin changes the grid, a card per cell, tiles in their cells, a grid it does not take refused');

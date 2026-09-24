/** Bounded history with independent document and editor-map actions.
 * Simple mode skips map actions; restoring either scope leaves the other alone.
 */
export type HistoryScope = 'document' | 'workspace';
type Entry<T> = { value: T; scope: HistoryScope };
export class DraftHistory<T> {
  private past: Entry<T>[] = [];
  private future: Entry<T>[] = [];
  clear() { this.past = []; this.future = []; }
  remember(value: T, scope: HistoryScope = 'document') {
    this.past.push({ value, scope });
    if (this.past.length > 100) this.past.shift();
    this.future = [];
  }
  counts(advanced: boolean) {
    const visible = (entry: Entry<T>) => advanced || entry.scope === 'document';
    return { undo: this.past.filter(visible).length, redo: this.future.filter(visible).length };
  }
  step(direction: 'undo' | 'redo', current: T, advanced: boolean): Entry<T> | undefined {
    const source = direction === 'undo' ? this.past : this.future;
    const target = direction === 'undo' ? this.future : this.past;
    const index = source.findLastIndex((entry) => advanced || entry.scope === 'document');
    if (index < 0) return;
    const [entry] = source.splice(index, 1);
    target.push({ value: current, scope: entry.scope });
    return entry;
  }
}

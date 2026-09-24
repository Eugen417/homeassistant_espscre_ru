/** Conflict recovery never changes the draft before an authoritative response. */
import type { PageDocument, PageGrid, PageLayout, PageWorkspace, Screen } from '../types';
import { sameGrid } from './pages';

export function savedDraft(record: Screen['page_document'], submitted: PageLayout, grid: PageGrid, workspace?: PageWorkspace): record is PageDocument {
  return record?.format === 'pages-v2' && sameGrid(record.sourceGrid, grid) &&
    JSON.stringify(record.layout) === JSON.stringify(submitted) &&
    (!workspace || JSON.stringify(record.workspace?.positions) === JSON.stringify(workspace.positions));
}

type State = { busy: boolean; conflict: boolean; reachable: boolean; selected: string | null;
  dirty: boolean; documentRevision: string | null; workspace: PageWorkspace };
type Services = { epoch: () => number; refresh: () => Promise<void>; screen: () => Screen | undefined;
  load: (screen: Screen) => void; acceptBase: (record: PageDocument) => void; save: () => Promise<void> };

export async function resolveConflict(choice: 'reload' | 'keep', state: State, services: Services) {
  if (state.busy || !state.conflict) return;
  const selected = state.selected, epoch = services.epoch();
  await services.refresh();
  if (!state.reachable || state.selected !== selected || services.epoch() !== epoch) return;
  const screen = services.screen(), record = screen?.page_document;
  if (!screen || record?.format !== 'pages-v2') return;
  if (choice === 'reload') {
    services.load(screen);
    state.dirty = false;
    return;
  }
  // Keep mine authorizes only this observed revision. A subsequent concurrent
  // save still fails the server's revision check.
  state.documentRevision = record.revision;
  state.workspace.revision = record.workspace?.revision || '';
  services.acceptBase(record);
  await services.save();
}

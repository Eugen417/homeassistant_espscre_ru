export type TileOptions = {
  display?: string;
  size?: string;
  controls?: string;
  inline?: string;
  tap?: string;
  icon?: string;
  background?: string;
  history_hours?: number;
  action?: { action: string; data?: Record<string, unknown> };
  [key: string]: unknown;
};
export type Tile = { entity: string; name: string; slot: number; options?: TileOptions };
export type HeaderItem = { type: string; entity?: string; content?: string; icon?: string; show?: string };
export type Layout = {
  title: string;
  tiles: Tile[];
  pages?: number;
  header?: { items: HeaderItem[] };
  settings?: Record<string, any>;
  [key: string]: unknown;
};
export type UpdateInfo = {
  available?: boolean; target?: string; state?: string; phase?: string; host?: string; profile?: string;
  result?: { state: string; message: string; time: number };
};
export type SettingsView = { owner: string; keys: string[]; values: Record<string, any>; unavailable: string[] };
export type Screen = {
  id: string; name: string; online: boolean; area?: string; firmware?: string; board?: string;
  layout: Layout; update?: UpdateInfo; settings?: SettingsView; delivery?: string; status?: string;
  alert_action?: string; dismiss_action?: string;
  // What the add-on reads from the firmware (app 0.2.78): its X.Y.Z (null when unknown), how many tiles it holds,
  // whether it draws full-page tiles, and whether it takes several tiles that go to the same page.
  firmware_known?: string | null; tile_limit?: number; full_page?: boolean; page_tiles_repeat?: boolean;
};
export type ChangelogSection = { app: string; firmware: string; lines: string[] };
export type Entity = { id: string; name: string; area?: string; device?: string; icon?: string; state?: string; tile?: boolean };
export type IconInfo = { name: string; cp: string; label: string };
export type Inventory = {
  csrf?: string;
  connected?: boolean;
  screens: Screen[];
  entities: Entity[];
  builtin?: Entity[];
  pending?: { friendly: string; file: string; installed?: boolean; downloaded?: boolean; api_key?: string }[];
  updates?: { target: string; busy?: boolean; pending?: number; auto?: boolean };
  // The CHANGELOG by release, newest first: only in the full inventory, not in the live payload (app 0.2.78).
  changelog?: ChangelogSection[];
  claude_skill?: { path: string; installed: boolean; current: boolean; restart?: boolean };
  icons?: {
    groups: { label: string; icons: IconInfo[] }[];
    builtin?: Record<string, string>; weather: Record<string, string>; sun: Record<string, string>;
    defaults: Record<string, string>; fallback: string; controls?: Record<string, string>;
  };
  backgrounds?: Record<string, { label: string; color?: string }>;
  controls?: Record<string, { default: string; choices: { key: string; label: string }[] }>;
  header?: {
    max_items?: number; min_firmware?: string;
    builtin: { type: string; label: string }[];
    contents: { key: string; label: string }[];
    shows: { key: string; label: string }[];
    suggestions?: Record<string, { item: HeaderItem; label: string; name?: string; area?: string; icon?: string }[]>;
  };
  alerts?: any;
  [key: string]: unknown;
};
export type Capability = { toggle: boolean; inline: boolean; controls: string[]; displays: string[] };
export type EntityAction = {
  action: string; name: string; description: string;
  fields: { key: string; name: string; required?: boolean; description?: string; example?: unknown; selector?: Record<string, any>; options?: string[] }[];
};

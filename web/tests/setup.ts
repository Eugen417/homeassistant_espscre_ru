// Every mounted component gets the editor's texts, as main.ts gives the page (app 0.2.90).
import { config, enableAutoUnmount } from "@vue/test-utils";
import { afterEach, vi } from "vitest";
import { i18n } from "../src/i18n";

config.global.plugins = [i18n];
enableAutoUnmount(afterEach);
// jsdom does not draw glyphs or implement modal dialogs. Geometry is verified
// with the real fonts in the browser and the firmware host renders.
HTMLCanvasElement.prototype.getContext = vi.fn(() => null) as any;
HTMLDialogElement.prototype.showModal = function () { this.setAttribute('open', ''); };

// Every mounted component gets the editor's texts, as main.ts gives the page (app 0.2.90).
import { config } from "@vue/test-utils";
import { i18n } from "../src/i18n";

config.global.plugins = [i18n];

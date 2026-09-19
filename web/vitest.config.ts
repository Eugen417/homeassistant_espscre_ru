import vue from "@vitejs/plugin-vue";
import { defineConfig } from "vitest/config";
import { editorTexts } from "./vite.config";

// Unit and component tests of the editor: the grid rules, the top bar rules, the store and the components,
// in jsdom. `npm test` runs them; `npm run test:watch` while developing. The components mount with the
// editor's texts (tests/setup.ts), in English.
export default defineConfig({
  plugins: [editorTexts(), vue()],
  test: {
    environment: "jsdom",
    include: ["tests/**/*.spec.ts"],
    setupFiles: ["tests/setup.ts"],
    restoreMocks: true,
  },
});

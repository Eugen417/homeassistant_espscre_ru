import vue from "@vitejs/plugin-vue";
import { defineConfig } from "vitest/config";

// Unit and component tests of the editor: the grid rules, the top bar rules, the store and the components,
// in jsdom. `npm test` runs them; `npm run test:watch` while developing.
export default defineConfig({
  plugins: [vue()],
  test: {
    environment: "jsdom",
    include: ["tests/**/*.spec.ts"],
    restoreMocks: true,
  },
});

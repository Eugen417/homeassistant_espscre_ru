import vue from "@vitejs/plugin-vue";
import { defineConfig } from "vite";

// The add-on serves the built page from screen_manager/app/static behind Home Assistant ingress,
// which lives on a sub path with a token. Everything is relative so the page works there and on
// a bare http://127.0.0.1:8099 (SCREEN_DEV). The Python server keeps serving /api; only the static
// files change. `npm run dev` proxies /api to a running server (the demo home or SCREEN_DEV).
export default defineConfig({
  base: "./",
  plugins: [vue()],
  build: {
    outDir: "../screen_manager/app/static",
    emptyOutDir: false,
    target: "es2022",
  },
  server: {
    port: 5173,
    proxy: { "/api": { target: "http://127.0.0.1:8099", changeOrigin: false } },
  },
});

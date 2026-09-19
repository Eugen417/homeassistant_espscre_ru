import vue from "@vitejs/plugin-vue";
import { defineConfig, type Plugin } from "vite";

// Every language is one file in screen_manager/translations (app 0.2.90), shared with the firmware and the add-on. The
// page needs only the language itself (_meta) and the editor's texts: the screens' and the add-on's texts stay out of
// the build. Before Vite's own JSON step, so the modules it makes hold these two sections only.
export function editorTexts(): Plugin {
  return {
    name: "esp-screens-editor-texts",
    enforce: "pre",
    transform(code, id) {
      if (!/[\\/]screen_manager[\\/]translations[\\/][^\\/?]+\.json$/.test(id)) return null;
      const { _meta, editor } = JSON.parse(code);
      return { code: JSON.stringify({ _meta, editor }), map: null };
    },
  };
}

// The add-on serves the built page from screen_manager/app/static behind Home Assistant ingress,
// which lives on a sub path with a token. Everything is relative so the page works there and on
// a bare http://127.0.0.1:8099 (SCREEN_DEV). The Python server keeps serving /api; only the static
// files change. `npm run dev` proxies /api to a running server (the demo home or SCREEN_DEV).
export default defineConfig({
  base: "./",
  plugins: [editorTexts(), vue()],
  // vue-i18n's switches (app 0.2.90): the Composition API only, texts compiled in the page, no devtools in the build.
  define: {
    __VUE_I18N_FULL_INSTALL__: true,
    __VUE_I18N_LEGACY_API__: false,
    __INTLIFY_PROD_DEVTOOLS__: false,
    __INTLIFY_DROP_MESSAGE_COMPILER__: false,
  },
  build: {
    outDir: "../screen_manager/app/static",
    emptyOutDir: false,
    target: "es2022",
  },
  server: {
    port: 5173,
    proxy: { "/api": { target: "http://127.0.0.1:8099", changeOrigin: false } },
    // The translations live beside the add-on, outside web/.
    fs: { allow: [".", "../screen_manager/translations"] },
  },
});

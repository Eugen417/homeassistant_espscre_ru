import { createApp } from "vue";
import App from "./App.vue";
import "./styles/tokens.css";
import "./styles/app.css";
import { boot } from "./store";

createApp(App).mount("#app");
boot();

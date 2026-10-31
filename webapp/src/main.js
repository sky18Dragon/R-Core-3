import { createApp } from "vue";
import { createPinia } from "pinia";
import vuetify from "./plugins/vuetify";
// Icon font is bundled (not a CDN) so the page works on a hotspot with no internet.
import "./styles/mdi-subset.css";
import router from "./router";
import App from "./App.vue";
import { onLocaleChange } from "./i18n";

const app = createApp(App);
const pinia = createPinia();

app.use(pinia);
app.use(router);
app.use(vuetify);
// Vuetify's own component text (pagination, data tables, …) follows the UI language.
onLocaleChange((l) => {
  vuetify.locale.current.value = l === "zh" ? "zhHans" : "en";
  document.documentElement.lang = l === "zh" ? "zh-CN" : "en";
});

app.mount("#app");

<script setup lang="ts">
// A plugin's README, as the app shows it: markdown without HTML, so a plugin cannot put a script or a form on the page.
// Links open in a new tab; a picture shows only from the plugin's own repository, at the commit the screen is pinned to.
import MarkdownIt from "markdown-it";
import { computed } from "vue";

const props = defineProps<{ source: string; repo: string }>();

const md = new MarkdownIt({ html: false, linkify: true, typographer: false, breaks: false });
// Raw files of the plugin's repository: a relative picture resolves there, any other picture is left out.
const raw = (repo: string) => {
  const match = repo.match(/^https:\/\/github\.com\/([^/]+)\/([^/]+)(?:\/tree\/([^/]+)\/(.*))?$/);
  return match ? `https://raw.githubusercontent.com/${match[1]}/${match[2]}/${match[3] || "HEAD"}/${match[4] ? `${match[4]}/` : ""}` : "";
};
md.renderer.rules.image = (tokens, index) => {
  const src = tokens[index].attrGet("src") || "";
  const base = raw(props.repo);
  const url = /^[a-z]+:/i.test(src) ? (base && src.startsWith(base) ? src : "") : base && `${base}${src.replace(/^\.?\//, "")}`;
  return url ? `<img src="${md.utils.escapeHtml(url)}" alt="${md.utils.escapeHtml(tokens[index].content)}" loading="lazy">` : "";
};
const linkOpen = md.renderer.rules.link_open || ((tokens, index, options, _env, self) => self.renderToken(tokens, index, options));
md.renderer.rules.link_open = (tokens, index, options, env, self) => {
  tokens[index].attrSet("target", "_blank");
  tokens[index].attrSet("rel", "noopener noreferrer");
  return linkOpen(tokens, index, options, env, self);
};
const html = computed(() => md.render(props.source));
</script>

<!-- markdown-it with html off escapes every tag in the source; what comes out is markdown's own elements only. -->
<!-- eslint-disable-next-line vue/no-v-html -->
<template><div class="pd-readme" v-html="html"></div></template>

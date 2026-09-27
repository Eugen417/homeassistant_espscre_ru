// The feedback card (app 0.3.10): "Not quite" points to the bug report on GitHub (app 0.4.13), where a problem gets
// the board, the versions and a log it needs to be fixed. A yes does not.
import { flushPromises, mount } from "@vue/test-utils";
import { beforeEach, describe, expect, it, vi } from "vitest";
import FeedbackPanel from "../src/components/FeedbackPanel.vue";
import type { Screen } from "../src/types";

function screen(): Screen {
  return {
    id: "living", name: "Living room", online: true, board: "guition", layout: { title: "Living room", tiles: [] },
    feedback: {
      available: true, ask: true, answered: false, shared: null, pending: null, state: "idle", problem: null, deleted: false,
      board: "guition", privacy: "https://example.com/privacy", versions: {},
    },
  } as unknown as Screen;
}

beforeEach(() => {
  vi.stubGlobal("fetch", vi.fn(() => Promise.resolve(new Response(JSON.stringify({ feedback: {} }), { status: 200 }))));
});

describe("feedback card", () => {
  it("sends a problem on to a GitHub bug report", async () => {
    const card = mount(FeedbackPanel, { props: { screen: screen(), mode: "card" } });
    expect(card.find(".fb-report").exists()).toBe(false);
    await card.findAll(".fb-actions .btn")[1].trigger("click");  // Not quite
    await flushPromises();
    const report = card.find(".fb-report a");
    expect(report.exists()).toBe(true);
    expect(report.attributes("href")).toBe("https://github.com/MaxGramser/homeassistant_espscreen/issues/new?template=bug_report.yml");
    expect(report.attributes("target")).toBe("_blank");
    expect(card.find(".fb-report").text()).toContain("GitHub");
  });
  it("does not after a yes", async () => {
    const card = mount(FeedbackPanel, { props: { screen: screen(), mode: "card" } });
    await card.findAll(".fb-actions .btn")[0].trigger("click");  // Yes, works well
    await flushPromises();
    await card.find(".fb-link").trigger("click");  // Add something
    expect(card.find(".fb-comment").exists()).toBe(true);
    expect(card.find(".fb-report").exists()).toBe(false);
  });
});

import { afterEach, expect, it, vi } from 'vitest';
import { mount } from '@vue/test-utils';
import { nextTick } from 'vue';
import MarqueeText from '../src/components/MarqueeText.vue';

afterEach(() => vi.unstubAllGlobals());
it('scrolls only overflowing text, remeasures after resize and cleans up', async () => {
  let resized = () => {};
  const disconnect = vi.fn();
  vi.stubGlobal('ResizeObserver', class {
    constructor(callback: () => void) { resized = callback; }
    observe() {} disconnect = disconnect;
  });
  const view = mount(MarqueeText, { props: { text: 'Long track title' } });
  const viewport = view.element;
  const text = view.get('.marquee-text').element;
  Object.defineProperty(viewport, 'clientWidth', { configurable: true, value: 100 });
  Object.defineProperty(text, 'scrollWidth', { configurable: true, value: 220 });
  resized(); await nextTick();
  expect(view.classes()).toContain('scrolling');
  expect(view.get('.marquee-copy').attributes('aria-hidden')).toBe('true');
  expect(view.attributes('style')).toContain('252px');
  Object.defineProperty(viewport, 'clientWidth', { value: 300 });
  resized(); await nextTick();
  expect(view.classes()).not.toContain('scrolling');
  expect(view.find('.marquee-copy').exists()).toBe(false);
  await view.setProps({ text: 'New track' });
  expect(view.attributes('title')).toBe('New track');
  view.unmount(); expect(disconnect).toHaveBeenCalledOnce();
});

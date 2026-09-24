import { mount } from '@vue/test-utils';
import { afterEach, expect, it } from 'vitest';
import { options } from 'floating-vue';
import HelpTip from '../src/components/HelpTip.vue';

const previousPositioning = options.themes.tooltip.positioningDisabled;
afterEach(() => { options.themes.tooltip.positioningDisabled = previousPositioning; });
it('opens by keyboard focus or tap, associates its text, and closes with Escape', async () => {
  options.themes.tooltip.positioningDisabled = true;
  const view = mount(HelpTip, { props: { text: 'A page keeps its own title.' }, attachTo: document.body });
  // jsdom has no layout. Exercise real library events; browser acceptance checks positioning.
  const button = view.get('button');
  await button.trigger('focus');
  await new Promise(resolve => setTimeout(resolve, 350));
  expect(button.attributes('aria-describedby')).toBeTruthy();
  expect(view.find('.v-popper__popper--shown').text()).toContain('A page keeps its own title.');
  await button.trigger('keydown', { key: 'Escape' });
  await new Promise(resolve => setTimeout(resolve, 350));
  expect(view.find('.v-popper__popper--shown').exists()).toBe(false);
  await button.trigger('click');
  await new Promise(resolve => setTimeout(resolve, 350));
  expect(view.find('.v-popper__popper--shown').exists()).toBe(true);
  await button.trigger('blur');
  await new Promise(resolve => setTimeout(resolve, 350));
  expect(view.find('.v-popper__popper--shown').exists()).toBe(false);
});

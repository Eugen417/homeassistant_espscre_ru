import { ref, watch } from 'vue';

/** Keep the user's in-progress spacing while saving canonical trimmed text.
 * An empty required field stays local until the user finishes typing; on blur
 * it shows the last valid value. The caller owns the focus-based undo group.
 */
export function textDraft(read: () => string, write: (value: string) => void, required = false) {
  const value = ref(read()), focused = ref(false);
  watch(read, next => { if (!focused.value) value.value = next; });
  return {
    value,
    focus() { focused.value = true; value.value = read(); },
    input(next: string) { value.value = next; if (!required || next.trim()) write(next.trim()); },
    blur() { focused.value = false; value.value = read(); },
  };
}

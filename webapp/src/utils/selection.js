import { computed, ref } from "vue";

// Photos picked for a batch action (delete, show on the frame). Shared by the
// photo wall, the album view and the selection bar; cleared when the view changes.
const picked = ref(new Map());

export function photoKey(image) {
  return `${image.album}/${image.filename}`;
}

export const selectedPhotos = computed(() => [...picked.value.values()]);
export const selectedCount = computed(() => picked.value.size);
export const selecting = computed(() => picked.value.size > 0);

export function isSelected(image) {
  return picked.value.has(photoKey(image));
}

export function toggleSelected(image) {
  const key = photoKey(image);
  const next = new Map(picked.value);
  if (next.has(key)) next.delete(key);
  else next.set(key, image);
  picked.value = next;
}

export function selectAll(images) {
  const next = new Map(picked.value);
  for (const image of images) next.set(photoKey(image), image);
  picked.value = next;
}

export function clearSelection() {
  if (picked.value.size) picked.value = new Map();
}

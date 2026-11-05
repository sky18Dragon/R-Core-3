import { defineStore } from "pinia";
import { ref, computed } from "vue";
import { turnUp } from "../utils/orientation";

export const useAppStore = defineStore("app", () => {
  // State
  const albums = ref([]);
  const selectedAlbum = ref("Default");
  const images = ref([]);
  // Every album's photos, for the photo wall: { [album]: [{ filename, album, thumbnail }] }
  const imagesByAlbum = ref({});
  const battery = ref({
    connected: false,
    level: 0,
    voltage: 0,
    charging: false,
  });
  // False until the first /api/system-info answer; defaults below are placeholders, so UI
  // that warns about missing storage must wait for it.
  const systemInfoLoaded = ref(false);
  const systemInfo = ref({
    width: 800,
    height: 480,
    has_sdcard: false,
    sdcard_inserted: false,
    has_flash_storage: false,
    storage_total: 0,
    storage_used: 0,
    version: "v1.0",
    project_name: "PhotoFrame",
    wakeup_key_name: "wake button",
    compile_time: "",
    compile_date: "",
    idf_version: "",
    board_name: "waveshare_photopainter_73",
    last_crash: null,
  });
  const loading = ref({
    albums: false,
    images: false,
    allImages: false,
    battery: false,
    systemInfo: false,
  });

  // Bumped whenever the picture on the panel changes, so views that show it
  // ("now showing") know to fetch it again.
  const displayVersion = ref(0);

  // API base URL (empty for same-origin)
  const API_BASE = "";

  // Getters
  const sortedAlbums = computed(() => {
    return [...albums.value].sort((a, b) => {
      if (a.name === "Default") return -1;
      if (b.name === "Default") return 1;
      return a.name.localeCompare(b.name);
    });
  });

  const currentAlbumImages = computed(() => images.value);

  // Grayscale panels ("gc16", and any future "gc8"/"gc4") hide the color-only
  // controls (saturation, 6-color palette calibration).
  const isGrayscale = computed(() => (systemInfo.value.display_type || "").startsWith("gc"));

  // Actions
  async function loadBatteryStatus() {
    try {
      const response = await fetch(`${API_BASE}/api/battery`);
      if (!response.ok || response.headers.get("content-type")?.includes("text/html")) {
        return;
      }
      const data = await response.json();
      battery.value = {
        connected: data.battery_connected,
        level: data.battery_level,
        voltage: data.battery_voltage,
        charging: data.charging,
      };
    } catch (_error) {
      console.log("Battery API not available (standalone mode)");
    }
  }

  async function loadAlbums() {
    loading.value.albums = true;
    try {
      if (!systemInfo.value.sdcard_inserted && !systemInfo.value.has_flash_storage) {
        albums.value = [{ name: "Default", enabled: true, image_count: 0 }];
        return;
      }
      const response = await fetch(`${API_BASE}/api/albums`);
      if (!response.ok || response.headers.get("content-type")?.includes("text/html")) {
        albums.value = [{ name: "Default", enabled: true, image_count: 0 }];
        return;
      }
      albums.value = await response.json();
    } catch (error) {
      console.log("Failed to load albums (standalone mode):", error);
      albums.value = [{ name: "Default", enabled: true, image_count: 0 }];
    } finally {
      loading.value.albums = false;
    }
  }

  async function loadImages(albumName) {
    loading.value.images = true;
    try {
      if (!systemInfo.value.sdcard_inserted && !systemInfo.value.has_flash_storage) {
        images.value = [];
        return;
      }
      const response = await fetch(`${API_BASE}/api/images?album=${encodeURIComponent(albumName)}`);
      if (!response.ok || response.headers.get("content-type")?.includes("text/html")) {
        images.value = [];
        return;
      }
      images.value = await response.json();
    } catch (error) {
      console.log("Failed to load images (standalone mode):", error);
      images.value = [];
    } finally {
      loading.value.images = false;
    }
  }

  async function fetchAlbumImages(albumName) {
    const response = await fetch(`${API_BASE}/api/images?album=${encodeURIComponent(albumName)}`);
    if (!response.ok || response.headers.get("content-type")?.includes("text/html")) return [];
    const list = await response.json();
    return Array.isArray(list) ? list : [];
  }

  // One request per album, in order, so a busy frame is not asked for all at once.
  async function loadAllImages() {
    loading.value.allImages = true;
    try {
      if (!systemInfo.value.sdcard_inserted && !systemInfo.value.has_flash_storage) {
        imagesByAlbum.value = {};
        return;
      }
      const next = {};
      for (const album of sortedAlbums.value) {
        try {
          next[album.name] = await fetchAlbumImages(album.name);
        } catch {
          next[album.name] = imagesByAlbum.value[album.name] || [];
        }
      }
      imagesByAlbum.value = next;
    } finally {
      loading.value.allImages = false;
    }
  }

  async function reloadAlbumImages(albumName) {
    try {
      imagesByAlbum.value = {
        ...imagesByAlbum.value,
        [albumName]: await fetchAlbumImages(albumName),
      };
    } catch {
      // Keep what is shown.
    }
  }

  function selectAlbum(albumName) {
    selectedAlbum.value = albumName;
    loadImages(albumName);
  }

  async function toggleAlbumEnabled(albumName, enabled) {
    try {
      const response = await fetch(
        `${API_BASE}/api/albums/enabled?name=${encodeURIComponent(albumName)}`,
        {
          method: "PUT",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ enabled }),
        }
      );
      if (response.ok) {
        await loadAlbums();
      }
    } catch (error) {
      console.error("Failed to toggle album:", error);
    }
  }

  async function createAlbum(name) {
    try {
      const response = await fetch(`${API_BASE}/api/albums`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name: name.trim() }),
      });
      if (response.ok) {
        await loadAlbums();
        return true;
      }
      return false;
    } catch (error) {
      console.error("Failed to create album:", error);
      return false;
    }
  }

  async function deleteAlbum(albumName) {
    try {
      const response = await fetch(`${API_BASE}/api/albums?name=${encodeURIComponent(albumName)}`, {
        method: "DELETE",
      });
      if (response.ok) {
        await loadAlbums();
        await loadSystemInfo();
        const rest = { ...imagesByAlbum.value };
        delete rest[albumName];
        imagesByAlbum.value = rest;
        if (selectedAlbum.value === albumName) {
          selectAlbum("Default");
        }
        return true;
      }
      return false;
    } catch (error) {
      console.error("Failed to delete album:", error);
      return false;
    }
  }

  // Which way is up for one photo (0/90/180/270). On success the lists update at
  // once; `redraw` says the frame shows this photo and should show it again.
  // Facts kept with one photo: `up` (which way is up) and/or `preset` (the
  // processing preset it was made with)
  async function setPhotoMeta(image, facts) {
    try {
      const response = await fetch(`${API_BASE}/api/photo`, {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ filepath: `${image.album}/${image.filename}`, ...facts }),
      });
      if (!response.ok) return { ok: false };
      const body = await response.json().catch(() => ({}));
      const list = imagesByAlbum.value[image.album];
      if (list) {
        imagesByAlbum.value = {
          ...imagesByAlbum.value,
          [image.album]: list.map((i) => (i.filename === image.filename ? { ...i, ...facts } : i)),
        };
      }
      return { ok: true, redraw: body.redraw === true };
    } catch (error) {
      console.error("Failed to update the photo:", error);
      return { ok: false };
    }
  }

  function setPhotoUp(image, up) {
    return setPhotoMeta(image, { up });
  }

  // Turns photos `delta` degrees (±90) and shows the one on the frame again
  // when its way round changed there. Returns how many were turned.
  async function rotatePhotos(list, delta) {
    let turned = 0;
    let redraw = null;
    for (const image of list) {
      const result = await setPhotoUp(image, turnUp(image.up, delta));
      if (result.ok) ++turned;
      if (result.redraw) redraw = image;
    }
    if (redraw) await displayImage(redraw.album, redraw.filename);
    return turned;
  }

  async function deleteOne(album, filename) {
    try {
      const response = await fetch(`${API_BASE}/api/delete`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ filepath: `${album}/${filename}` }),
      });
      return response.ok;
    } catch (error) {
      console.error("Failed to delete image:", error);
      return false;
    }
  }

  async function deleteImage(album, filename) {
    const ok = await deleteOne(album, filename);
    if (ok) {
      await loadImages(selectedAlbum.value);
      await reloadAlbumImages(album);
      await loadSystemInfo();
    }
    return ok;
  }

  // Deletes one by one and refreshes once; returns how many were deleted.
  async function deleteImages(list) {
    let deleted = 0;
    for (const image of list) {
      if (await deleteOne(image.album, image.filename)) ++deleted;
    }
    const albumsTouched = [...new Set(list.map((image) => image.album))];
    for (const album of albumsTouched) await reloadAlbumImages(album);
    if (albumsTouched.includes(selectedAlbum.value)) await loadImages(selectedAlbum.value);
    await loadSystemInfo();
    return deleted;
  }

  async function displayImage(album, filename) {
    try {
      const fullPath = `${album}/${filename}`;
      const response = await fetch(`${API_BASE}/api/display`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ filepath: fullPath }),
      });
      if (response.ok) displayVersion.value++;
      return response.ok;
    } catch (error) {
      console.error("Failed to display image:", error);
      return false;
    }
  }

  async function enterSleep() {
    try {
      const response = await fetch(`${API_BASE}/api/sleep`, { method: "POST" });
      return response.ok;
    } catch (error) {
      console.error("Failed to enter sleep:", error);
      return false;
    }
  }

  async function rotateNow() {
    try {
      const response = await fetch(`${API_BASE}/api/rotate`, { method: "POST" });
      if (response.ok) displayVersion.value++;
      return response.ok;
    } catch (error) {
      console.error("Failed to rotate:", error);
      return false;
    }
  }

  async function keepAlive() {
    try {
      const response = await fetch(`${API_BASE}/api/keep_alive`, { method: "POST" });
      return response.ok;
    } catch (error) {
      console.error("Failed to send keep-alive:", error);
      return false;
    }
  }

  async function loadSystemInfo() {
    loading.value.systemInfo = true;
    try {
      const response = await fetch(`${API_BASE}/api/system-info`);
      if (response.ok) {
        systemInfo.value = await response.json();
        systemInfoLoaded.value = true;
      }
    } catch (error) {
      console.error("Failed to load system info:", error);
    } finally {
      loading.value.systemInfo = false;
    }
  }

  return {
    // State
    albums,
    selectedAlbum,
    images,
    imagesByAlbum,
    battery,
    systemInfo,
    systemInfoLoaded,
    loading,
    displayVersion,
    // Getters
    sortedAlbums,
    currentAlbumImages,
    isGrayscale,
    // Actions
    loadBatteryStatus,
    loadAlbums,
    loadImages,
    loadAllImages,
    reloadAlbumImages,
    selectAlbum,
    toggleAlbumEnabled,
    createAlbum,
    deleteAlbum,
    deleteImage,
    deleteImages,
    setPhotoUp,
    setPhotoMeta,
    rotatePhotos,
    displayImage,
    enterSleep,
    rotateNow,
    keepAlive,
    loadSystemInfo,
  };
});

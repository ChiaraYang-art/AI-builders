import { computed, ref } from "vue";

import {
  finishWalkDiary,
  startWalk,
  uploadWalkAudio,
  uploadWalkPhoto,
} from "../api/walk.js";

export const WALK_SESSION_KEY = Symbol("walkSession");

export function useWalkSession(refreshLatest) {
  const walkId = ref(null);
  const walkType = ref(null);
  const localActiveWalk = ref(null);
  const uploadError = ref("");
  const isUploading = ref(false);
  const isFinishing = ref(false);
  const isRecording = ref(false);

  let mediaRecorder = null;
  let recordedChunks = [];

  const activeWalk = computed(() => localActiveWalk.value);

  async function startSession(type) {
    uploadError.value = "";
    const session = await startWalk(type);
    walkId.value = session.walk_id;
    walkType.value = type;
    localActiveWalk.value = session;

    if (typeof refreshLatest === "function") {
      await refreshLatest();
    }

    return session;
  }

  function syncFromLatest(latest) {
    const remote = latest?.active_walk;
    if (remote?.walk_id) {
      walkId.value = remote.walk_id;
      walkType.value = remote.type;
      localActiveWalk.value = remote;
    }
  }

  async function uploadPhoto(file) {
    if (!walkId.value || !file) {
      return null;
    }

    isUploading.value = true;
    uploadError.value = "";

    try {
      const result = await uploadWalkPhoto(walkId.value, file);
      localActiveWalk.value = result.active_walk || localActiveWalk.value;

      if (typeof refreshLatest === "function") {
        await refreshLatest();
      }

      return result;
    } catch (error) {
      uploadError.value = error instanceof Error ? error.message : String(error);
      throw error;
    } finally {
      isUploading.value = false;
    }
  }

  async function uploadAudioBlob(blob, filename) {
    if (!walkId.value || !blob) {
      return null;
    }

    isUploading.value = true;
    uploadError.value = "";

    try {
      const result = await uploadWalkAudio(walkId.value, blob, filename);
      localActiveWalk.value = result.active_walk || localActiveWalk.value;

      if (typeof refreshLatest === "function") {
        await refreshLatest();
      }

      return result;
    } catch (error) {
      uploadError.value = error instanceof Error ? error.message : String(error);
      throw error;
    } finally {
      isUploading.value = false;
    }
  }

  async function startRecording() {
    if (isRecording.value || isUploading.value) {
      return;
    }

    uploadError.value = "";

    if (!navigator.mediaDevices?.getUserMedia) {
      uploadError.value = "当前浏览器不支持录音。";
      return;
    }

    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
    recordedChunks = [];
    mediaRecorder = new MediaRecorder(stream);

    mediaRecorder.ondataavailable = (event) => {
      if (event.data.size > 0) {
        recordedChunks.push(event.data);
      }
    };

    mediaRecorder.onstop = async () => {
      stream.getTracks().forEach((track) => track.stop());

      if (recordedChunks.length === 0) {
        isRecording.value = false;
        return;
      }

      const blob = new Blob(recordedChunks, { type: mediaRecorder.mimeType || "audio/webm" });
      recordedChunks = [];
      isRecording.value = false;

      try {
        await uploadAudioBlob(blob, `sound_${Date.now()}.webm`);
      } catch {
        // uploadError already set
      }
    };

    mediaRecorder.start();
    isRecording.value = true;
  }

  function stopRecording() {
    if (mediaRecorder && mediaRecorder.state !== "inactive") {
      mediaRecorder.stop();
    }
  }

  async function finishSession() {
    if (!walkId.value) {
      throw new Error("没有进行中的散步。");
    }

    isFinishing.value = true;
    uploadError.value = "";

    try {
      const diary = await finishWalkDiary(walkId.value);
      walkId.value = null;
      walkType.value = null;
      localActiveWalk.value = null;

      if (typeof refreshLatest === "function") {
        await refreshLatest();
      }

      return diary;
    } catch (error) {
      uploadError.value = error instanceof Error ? error.message : String(error);
      throw error;
    } finally {
      isFinishing.value = false;
    }
  }

  return {
    walkId,
    walkType,
    activeWalk,
    uploadError,
    isUploading,
    isFinishing,
    isRecording,
    startSession,
    syncFromLatest,
    uploadPhoto,
    uploadAudioBlob,
    startRecording,
    stopRecording,
    finishSession,
  };
}

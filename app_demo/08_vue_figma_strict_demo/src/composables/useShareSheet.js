import { ref } from "vue";

const toast = ref("");
const shareSheet = ref(false);

export function useShareSheet() {
  function saveShare() {
    toast.value = "图片已保存";
    shareSheet.value = false;
  }

  function openShare() {
    toast.value = "";
    shareSheet.value = true;
  }

  function clearToast() {
    toast.value = "";
    shareSheet.value = false;
  }

  return {
    toast,
    shareSheet,
    saveShare,
    openShare,
    clearToast,
  };
}

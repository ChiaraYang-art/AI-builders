const assetBase = "/assets/figma/";

export function asset(fileName) {
  return assetBase + fileName.split("/").map(encodeURIComponent).join("/");
}

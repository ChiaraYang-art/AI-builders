export function dayClass(day) {
  const doneDays = new Set([7, 18, 28]);

  if (day === 28) {
    return "day today";
  }

  if (day === 10) {
    return "day sound";
  }

  if (day === 3 || day === 21) {
    return "day sunny";
  }

  if (day === 14 || day === 24) {
    return "day peach";
  }

  if (doneDays.has(day)) {
    return "day done";
  }

  return "day";
}

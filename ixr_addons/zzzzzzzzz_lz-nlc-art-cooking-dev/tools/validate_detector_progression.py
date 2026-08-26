#!/usr/bin/env python3
"""Validate exact detector lists together with IX-Ray af_rank gates."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ADDON_ROOT = Path(__file__).resolve().parents[1]
ARTIFACT_PATHS = (
    ADDON_ROOT / "configs" / "mod_system_lz_nlc_artifacts_full.ltx",
    ADDON_ROOT / "configs" / "mod_system_lz_nlc_natural_artifacts.ltx",
)
DETECTORS_PATH = (
    ADDON_ROOT / "configs" / "mod_system_lz_nlc_detector_classes.ltx"
)
LEGACY_DETECTORS_PATH = (
    ADDON_ROOT / "configs" / "mod_system_lz_nlc_art_cooking.ltx"
)

FAMILIES = ("spirit", "cry", "dik", "kol", "babka", "pudd", "armor")
NATURAL_CLASSES = {
    1: (
        "af_medusa", "af_vyvert", "af_blood", "af_electra_sparkler",
        "af_rusty_thorn", "af_drops", "af_ameba_slime",
    ),
    2: (
        "af_cristall_flower", "af_gravi", "af_mincer_meat",
        "af_dummy_spring", "af_rusty_kristall", "af_fireball",
        "af_ameba_slug",
    ),
    3: (
        "af_night_star", "af_gold_fish", "af_electra_moonlight",
        "af_cristall", "af_ameba_mica",
    ),
    4: (
        "af_soul", "af_electra_flash", "af_rusty_sea-urchin",
        "af_dummy_kolobok", "lz_nlc_af_dummy_glassbeads",
        "lz_nlc_af_dummy_dummy", "af_dummy_pellicle",
    ),
}
NATURAL = tuple(
    section
    for artifact_class in range(1, 5)
    for section in NATURAL_CLASSES[artifact_class]
)
COOKED = {
    stage: tuple(f"af_{family}_{stage}" for family in FAMILIES)
    for stage in range(1, 5)
}
ECHO = NATURAL + (
    "af_dummy_buliz", "af_quest_b14_twisted",
    "jup_b1_half_artifact", "af_compass",
)
BEAR = ECHO + COOKED[1] + COOKED[2] + (
    "af_dummy_glassbeads", "af_fuzz_kolobok", "af_dummy_battery",
)
VELES = BEAR + COOKED[3] + (
    "af_eye", "af_fire", "af_baloon", "af_glass",
    "af_dummy_dummy", "af_ice",
)
SVAROG = VELES + COOKED[4] + (
    "af_dummy_simbion", "lz_nlc_af_dummy_battery", "af_oasis_heart",
)
DETECTORS = {
    "detector_simple": (1, ECHO),
    "detector_advanced": (2, BEAR),
    "detector_elite": (3, VELES),
    "detector_scientific": (3, SVAROG),
}
EXPECTED_RANKS = {
    **{section: 1 for section in NATURAL},
    "af_dummy_buliz": 1,
    "af_dummy_simbion": 1,
    "lz_nlc_af_dummy_battery": 1,
    **{section: 1 for section in COOKED[1]},
    **{section: 2 for section in COOKED[2]},
    **{section: 3 for section in COOKED[3]},
    **{section: 3 for section in COOKED[4]},
}


def parse_ltx(paths: tuple[Path, ...]) -> dict[str, dict[str, str]]:
    sections: dict[str, dict[str, str]] = {}
    for path in paths:
        current: dict[str, str] | None = None
        for raw_line in path.read_text(encoding="utf-8-sig").splitlines():
            line = raw_line.split(";", 1)[0].strip()
            if not line:
                continue
            match = re.fullmatch(r"!?\[([^]]+)](?::.*)?", line)
            if match:
                current = sections.setdefault(match.group(1).strip(), {})
                continue
            if current is not None and "=" in line:
                key, value = line.split("=", 1)
                current[key.strip()] = value.strip()
    return sections


def indexed_values(section: dict[str, str], prefix: str) -> dict[int, str]:
    result: dict[int, str] = {}
    pattern = re.compile(rf"{re.escape(prefix)}(\d+)_?$")
    for key, value in section.items():
        match = pattern.fullmatch(key)
        if match:
            result[int(match.group(1))] = value
    return result


def main() -> int:
    artifacts = parse_ltx(ARTIFACT_PATHS)
    # Mirror the active DLTX overlay: the older partial append loads first and
    # the exact class file must overwrite it without leaving a stale tail.
    detectors = parse_ltx((LEGACY_DETECTORS_PATH, DETECTORS_PATH))
    errors: list[str] = []
    ranks: dict[str, int] = {}

    if tuple(NATURAL_CLASSES[1]) == tuple(NATURAL_CLASSES[4]):
        errors.append("natural class definitions unexpectedly coincide")

    for artifact, expected_rank in EXPECTED_RANKS.items():
        raw_rank = artifacts.get(artifact, {}).get("af_rank")
        try:
            actual_rank = int(raw_rank) if raw_rank is not None else None
        except ValueError:
            actual_rank = None
        if actual_rank != expected_rank:
            errors.append(
                f"{artifact}: af_rank={raw_rank!r}, expected {expected_rank}"
            )
        else:
            ranks[artifact] = actual_rank

    for detector, (gate, expected_classes) in DETECTORS.items():
        section = detectors.get(detector)
        if section is None:
            errors.append(f"{detector}: exact patch section is missing")
            continue
        classes = indexed_values(section, "af_class_")
        sounds = indexed_values(section, "af_sound_")
        freqs = indexed_values(section, "af_freq_")
        expected_indices = list(range(1, len(expected_classes) + 1))
        if sorted(classes) != expected_indices:
            errors.append(f"{detector}: class indices are not exactly 1..{len(expected_classes)}")
        if sorted(sounds) != expected_indices:
            errors.append(f"{detector}: sound indices are not continuous")
        if sorted(freqs) != expected_indices:
            errors.append(f"{detector}: frequency indices are not continuous")
        actual = tuple(classes.get(index) for index in expected_indices)
        if actual != expected_classes:
            errors.append(f"{detector}: artifact order/content differs from the approved list")

        blocked = [
            artifact for artifact in expected_classes
            if artifact in ranks and ranks[artifact] > gate
        ]
        if blocked:
            errors.append(
                f"{detector}: rank gate {gate} blocks {', '.join(blocked)}"
            )
        print(f"OK: {detector}: {len(expected_classes)} exact entries, gate {gate}")

    if not set(COOKED[4]).isdisjoint(set(VELES)):
        errors.append("Veles must not detect Absolutes")
    if not {"af_dummy_simbion", "lz_nlc_af_dummy_battery"}.isdisjoint(set(VELES)):
        errors.append("unique Symbion/Battery must remain Svarog-only")
    if not set(COOKED[3]).isdisjoint(set(BEAR)):
        errors.append("Bear must not detect hypermodificats")

    if errors:
        print("Detector progression validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("OK: Echo I-IV; Bear +modificat/+meso; Veles +hyper; Svarog all")
    print("OK: Symbion and Battery are Svarog-only; Boulder remains recoverable")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

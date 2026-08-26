#!/usr/bin/env python3
"""Validate the approved 26-artifact NLC natural class system."""

from __future__ import annotations

import re
import struct
import sys
from pathlib import Path
from xml.etree import ElementTree


ADDON = Path(__file__).resolve().parents[1]
NATURAL_CONFIG = ADDON / "configs" / "mod_system_lz_nlc_natural_artifacts.ltx"
COOKING_CONFIG = ADDON / "configs" / "mod_system_lz_nlc_artifacts_full.ltx"
ATLAS = ADDON / "textures" / "ui" / "ui_lz_nlc_natural_artifacts.dds"
QA_CONFIG = ADDON / "configs" / "mod_system_lz_nlc_natural_artifacts_qa.ltx"
QA_MOD_SCRIPT = ADDON / "configs" / "mod_script_lz_nlc_natural_artifacts_qa.ltx"
QA_SCRIPT = ADDON / "scripts" / "lz_nlc_natural_artifacts_qa.script"
ACTIVE_FACADE = ADDON / "scripts" / "lz_new_mechanics.script"

CLASSES = {
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
TARGET = tuple(
    section for artifact_class in range(1, 5) for section in CLASSES[artifact_class]
)
NATURAL_FILE_TARGETS = set(TARGET) - {
    "af_dummy_spring", "af_dummy_kolobok", "lz_nlc_af_dummy_glassbeads",
    "lz_nlc_af_dummy_dummy", "af_dummy_pellicle",
}
FULL_NLC_PORTS = {
    "af_rusty_thorn", "af_rusty_kristall", "af_rusty_sea-urchin",
    "af_drops", "af_ameba_slime", "af_ameba_slug", "af_ameba_mica",
}
COP_PATCHES = NATURAL_FILE_TARGETS - FULL_NLC_PORTS
QA_ORDER = (
    "af_medusa", "af_cristall_flower", "af_night_star",
    "af_vyvert", "af_gravi", "af_gold_fish",
    "af_blood", "af_mincer_meat", "af_soul",
    "af_electra_sparkler", "af_dummy_spring",
    "af_electra_moonlight", "af_electra_flash",
    "af_rusty_thorn", "af_rusty_kristall", "af_rusty_sea-urchin",
    "af_drops", "af_fireball", "af_cristall",
    "af_ameba_slime", "af_ameba_slug", "af_ameba_mica",
    "af_dummy_kolobok", "lz_nlc_af_dummy_glassbeads",
    "lz_nlc_af_dummy_dummy", "af_dummy_pellicle",
)
COST = {1: "6000", 2: "10000", 3: "16000", 4: "16000"}
IMMUNITIES = {
    "burn_immunity", "strike_immunity", "shock_immunity",
    "wound_immunity", "radiation_immunity", "telepatic_immunity",
    "chemical_burn_immunity", "explosion_immunity", "fire_wound_immunity",
}
MODELS = {
    "af_rusty_thorn": "physics\\anomaly\\artefact_needles1.ogf",
    "af_rusty_kristall": "physics\\anomaly\\artefact_needles2.ogf",
    "af_rusty_sea-urchin": "physics\\anomaly\\nlc_natural\\artefact_rusty_hairs.ogf",
    "af_drops": "physics\\anomaly\\artefact_kaply.ogf",
    "af_ameba_slime": "physics\\anomaly\\artefact_ameba3.ogf",
    "af_ameba_slug": "physics\\anomaly\\artefact_ameba2.ogf",
    "af_ameba_mica": "physics\\anomaly\\artefact_ameba1.ogf",
}
REQUIRED_TEXT_IDS = {
    *(f"st_lz_nlc_{section.replace('-', '_')}_{suffix}"
      for section in FULL_NLC_PORTS for suffix in ("name", "descr")),
    "st_lz_nlc_af_soul_descr",
    "st_lz_nlc_af_electra_flash_descr",
}


def parse_ltx(path: Path) -> tuple[dict[str, dict[str, str]], dict[str, bool]]:
    sections: dict[str, dict[str, str]] = {}
    patches: dict[str, bool] = {}
    current: dict[str, str] | None = None
    for raw_line in path.read_text(encoding="utf-8-sig").splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        match = re.fullmatch(r"(!?)\[([^]]+)](?::.*)?", line)
        if match:
            name = match.group(2).strip()
            current = sections.setdefault(name, {})
            patches[name] = bool(match.group(1))
            continue
        if current is not None and "=" in line:
            key, value = line.split("=", 1)
            current[key.strip()] = value.strip()
    return sections, patches


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def main() -> int:
    errors: list[str] = []
    natural, natural_patches = parse_ltx(NATURAL_CONFIG)
    cooking, _ = parse_ltx(COOKING_CONFIG)
    qa_sections, qa_patches = parse_ltx(QA_CONFIG)
    merged = {**cooking, **natural}

    if len(TARGET) != 26 or len(set(TARGET)) != 26:
        fail(errors, "natural class table must contain 26 unique sections")
    if tuple(map(len, CLASSES.values())) != (7, 7, 5, 7):
        fail(errors, "class counts must be 7/7/5/7")

    for artifact_class, artifacts in CLASSES.items():
        for section in artifacts:
            values = merged.get(section)
            if values is None:
                fail(errors, f"missing natural section [{section}]")
                continue
            if values.get("af_rank") != "1":
                fail(errors, f"{section}: technical af_rank must be 1")
            if values.get("cost") != COST[artifact_class]:
                fail(errors, f"{section}: class {artifact_class} cost must be {COST[artifact_class]}")
            if section in COP_PATCHES and not natural_patches.get(section):
                fail(errors, f"{section}: CoP analogue must remain a DLTX patch")
            if section in COP_PATCHES and "visual" in natural.get(section, {}):
                fail(errors, f"{section}: CoP world visual must remain inherited")
            if section in FULL_NLC_PORTS and natural_patches.get(section):
                fail(errors, f"{section}: missing NLC artifact must be a full section")

            absorb = values.get("hit_absorbation_sect", "")
            absorb_values = merged.get(absorb)
            if absorb_values is None:
                fail(errors, f"{section}: missing absorption section [{absorb}]")
            elif set(absorb_values) != IMMUNITIES:
                fail(errors, f"{section}: absorption keys are incomplete")
            else:
                for key, raw in absorb_values.items():
                    try:
                        value = float(raw)
                    except ValueError:
                        fail(errors, f"{section}: invalid {key}={raw}")
                        continue
                    if abs(value) > 0.10:
                        fail(errors, f"{section}: unadapted immunity {key}={value}")

            for key in ("radiation_restore_speed", "health_restore_speed",
                        "power_restore_speed", "bleeding_restore_speed"):
                raw = values.get(key, "0")
                try:
                    value = float(raw)
                except ValueError:
                    fail(errors, f"{section}: invalid {key}={raw}")
                    continue
                if abs(value) > 0.01:
                    fail(errors, f"{section}: unadapted restore value {key}={value}")

    for section, visual in MODELS.items():
        if natural.get(section, {}).get("visual") != visual:
            fail(errors, f"{section}: visual must be {visual}")
            continue
        model_path = ADDON / "meshes" / Path(*visual.split("\\"))
        if not model_path.is_file():
            fail(errors, f"{section}: missing model {model_path.relative_to(ADDON)}")
        elif b"link" not in model_path.read_bytes():
            fail(errors, f"{section}: model does not expose inherited link bone")

    for language in ("rus", "eng"):
        xml_path = ADDON / "configs" / "text" / language / "st_lz_nlc_natural_artifacts.xml"
        try:
            xml_text = xml_path.read_text(encoding="utf-8-sig")
            ids = {
                node.attrib.get("id", "")
                for node in ElementTree.parse(xml_path).getroot().findall("string")
            }
        except (OSError, ElementTree.ParseError) as exc:
            fail(errors, f"{language} localization: {exc}")
            continue
        missing = REQUIRED_TEXT_IDS - ids
        if missing:
            fail(errors, f"{language} localization misses {sorted(missing)}")
        retired_wording = (
            "Уровень природного артефакта"
            if language == "rus" else "Natural artefact tier"
        )
        if retired_wording in xml_text:
            fail(errors, f"{language} localization still mixes class and tier wording")

    try:
        data = ATLAS.read_bytes()
        height, width = struct.unpack_from("<II", data, 12)
        if data[:4] != b"DDS " or (width, height, data[84:88]) != (512, 256, b"DXT5"):
            fail(errors, "natural icon atlas must be 512x256 DXT5 DDS")
    except (OSError, struct.error) as exc:
        fail(errors, f"icon atlas: {exc}")

    qa_stash = qa_sections.get("lz_nlc_natural_qa_art_stash", {})
    if qa_patches.get("lz_nlc_natural_qa_art_stash"):
        fail(errors, "natural QA chest must be a full section")
    if qa_stash.get("visual") != "dynamics\\box\\box_wood_01.ogf":
        fail(errors, "natural QA chest uses an unexpected model")

    qa_script = QA_SCRIPT.read_text(encoding="utf-8-sig")
    items_match = re.search(r"local items\s*=\s*{(.*?)}", qa_script, re.DOTALL)
    qa_items = re.findall(r'"((?:af|lz_nlc_af)_[^"]+)"', items_match.group(1)) if items_match else []
    if tuple(qa_items) != QA_ORDER:
        fail(errors, "natural QA chest must contain the exact 26 class I-IV artifacts")
    if "\trevision = 2," not in qa_script:
        fail(errors, "natural QA chest revision must be 2")
    if "pf_stash_qa.is_developer_mode() == true" not in qa_script:
        fail(errors, "natural QA runtime is not fail-closed to developer mode")

    mod_script = QA_MOD_SCRIPT.read_text(encoding="utf-8-sig")
    facade = ACTIVE_FACADE.read_text(encoding="utf-8-sig")
    if ">script = lz_nlc_natural_artifacts_qa" not in mod_script:
        fail(errors, "natural QA module is not imported")
    if facade.count("lz_nlc_natural_artifacts_qa.on_game_load()") != 1:
        fail(errors, "active facade must delegate natural QA load once")
    if facade.count("lz_nlc_natural_artifacts_qa.update(now)") != 1:
        fail(errors, "active facade must delegate natural QA update once")

    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print("OK: 26 natural artifacts in classes I/II/III/IV = 7/7/5/7")
    print("OK: all natural classes pass Echo rank gate and remain below modificat price")
    print("OK: CoP analogues keep world models; seven missing artifacts keep NLC models")
    print("OK: developer-only third QA chest contains the exact 26 artifacts")
    return 0


if __name__ == "__main__":
    sys.exit(main())

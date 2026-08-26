#!/usr/bin/env python3
"""Static checks for natural classes, cooking, trade and Guardian restrictions."""

from __future__ import annotations

import configparser
import json
import re
import sys
from collections import Counter
from pathlib import Path
from xml.etree import ElementTree


ADDON = Path(__file__).resolve().parents[1]
PRODUCT = ADDON.parents[1]
TABLE = ADDON / "configs" / "misc" / "lz_nlc_art_spawn.ltx"
FULL_ARTIFACTS = ADDON / "configs" / "mod_system_lz_nlc_artifacts_full.ltx"
NATURAL_ARTIFACTS = ADDON / "configs" / "mod_system_lz_nlc_natural_artifacts.ltx"
REGISTRY = ADDON / "scripts" / "lz_nlc_anomaly_registry.script"
SPAWN_CONFIG = ADDON / "scripts" / "lz_nlc_art_spawn_config.script"
SPAWN_RUNTIME = ADDON / "scripts" / "lz_nlc_art_spawn.script"
NATIVE_BINDER = ADDON / "scripts" / "bind_anomaly_zone.script"
GUARDIAN = ADDON / "scripts" / "pf_monolith_guardian.script"
COOKING_FACADE = ADDON / "scripts" / "lz_nlc_art_cooking.script"
COOKING_RUNTIME = ADDON / "scripts" / "lz_nlc_art_cooking_runtime.script"
HERMANN_RECIPES = ADDON / "scripts" / "pf_hermann_art_recipe_dialog.script"
DIALOGS = ADDON / "configs" / "gameplay" / "dialogs_pripyat_full.xml"
RUS_TEXT = ADDON / "configs" / "text" / "rus" / "st_lz_nlc_art_cooking.xml"
ENG_TEXT = ADDON / "configs" / "text" / "eng" / "st_lz_nlc_art_cooking.xml"
RUS_NATURAL_TEXT = (
    ADDON / "configs" / "text" / "rus" / "st_lz_nlc_natural_artifacts.xml"
)
ENG_NATURAL_TEXT = (
    ADDON / "configs" / "text" / "eng" / "st_lz_nlc_natural_artifacts.xml"
)
FIELD_ROOT = ADDON / "configs" / "scripts" / "pripyat_full" / "anomaly"
BEARD_TRADE = ADDON / "configs" / "misc" / "trade" / "trade_zat_a2_barmen.ltx"
HERMANN_TRADE = PRODUCT / "gamedata" / "configs" / "misc" / "trade" / "trade_pf_bunker_hermann.ltx"
HAWAIIAN_TRADE = PRODUCT / "gamedata" / "configs" / "misc" / "trade" / "trade_pf_yanov_hawaiian.ltx"
TASKBOARD_ORDERS = ADDON / "configs" / "misc" / "lz_orders.ltx"
TASKBOARD_SCRIPT = ADDON / "scripts" / "lz_orders.script"
ANOMALY_ADDON = ADDON.parent / "zzzzzzzzzz_pf-procedural-anomalies-test"
ANOMALY_PROTOTYPE = ANOMALY_ADDON / "prototype_static_verification.json"
ANOMALY_ZONES = ANOMALY_ADDON / "configs" / "zones" / "pf_procedural_anomaly_zones.ltx"
ANOMALY_SERVER = ANOMALY_ADDON / "scripts" / "se_zones.script"

FAMILIES = ("spirit", "cry", "dik", "kol", "babka", "pudd", "armor")
COOKING_ROLES = {
    "spirit": ("zharka", "galant", "zharka", "galant"),
    "cry": ("galant", "zharka", "buzz", "galant"),
    "dik": ("zharka", "galant", "buzz", "mosquito_bald"),
    "kol": ("mosquito_bald", "mincer", "buzz", "zharka"),
    "babka": ("galant", "buzz", "mincer", "zharka"),
    "pudd": ("zharka", "buzz", "zharka", "buzz"),
    "armor": ("mosquito_bald", "mincer", "buzz", "zharka"),
}
ZONE_FAMILIES = {"psi", "electric", "chemical", "gravitational", "thermal"}
COOKING_PREFIX_ROLES = {
    "zone_burning_fuzz": "buzz",
    "zone_buzz": "buzz",
    "zone_field_acidic": "buzz",
    "zone_field_thermal": "zharka",
    "zone_mine_acidic": "buzz",
    "zone_mine_electric": "galant",
    "zone_mine_gravitational_weak": "mosquito_bald",
    "zone_mine_gravitational_average": "gravi_zone",
    "zone_mine_gravitational_strong": "mincer",
    "zone_mosquito_bald": "mosquito_bald",
    "zone_gravi_zone": "gravi_zone",
    "zone_mincer": "mincer",
    "zone_mine_thermal": "zharka",
    "zone_zharka_static": "zharka",
}
PROCEDURAL_COOKING_ROLES = {
    "pf_proc_gravitational_weak": "mosquito_bald",
    "pf_proc_gravitational_average": "gravi_zone",
    "pf_proc_gravitational_strong": "mincer",
    "pf_proc_electric_weak": "galant",
    "pf_proc_electric_average": "galant",
    "pf_proc_electric_strong": "galant",
    "pf_proc_chemical_weak": "buzz",
    "pf_proc_chemical_average": "buzz",
    "pf_proc_chemical_strong": "buzz",
    "pf_proc_thermal_weak": "zharka",
    "pf_proc_thermal_average": "zharka",
    "pf_proc_thermal_strong": "zharka",
}
GRAVITY_LORE_NAMES = {
    "zone_mine_gravitational_weak": "st_lz_nlc_anomaly_springboard",
    "zone_mine_gravitational_average": "st_lz_nlc_anomaly_vortex",
    "zone_mine_gravitational_strong": "st_lz_nlc_anomaly_whirligig",
    "zone_mosquito_bald": "st_lz_nlc_anomaly_springboard",
    "zone_gravi_zone": "st_lz_nlc_anomaly_vortex",
    "zone_mincer": "st_lz_nlc_anomaly_whirligig",
}
EXPECTED_ZONE_COUNTS = {
    "gravitational": 9, "chemical": 8, "electric": 8,
    "thermal": 8, "psi": 8,
}
NATURAL_CLASSES = {
    1: {
        "af_medusa", "af_vyvert", "af_blood", "af_electra_sparkler",
        "af_rusty_thorn", "af_drops", "af_ameba_slime",
    },
    2: {
        "af_cristall_flower", "af_gravi", "af_mincer_meat",
        "af_dummy_spring", "af_rusty_kristall", "af_fireball",
        "af_ameba_slug",
    },
    3: {
        "af_night_star", "af_gold_fish", "af_electra_moonlight",
        "af_cristall", "af_ameba_mica", "af_dummy_simbion",
        "lz_nlc_af_dummy_battery",
    },
    4: {
        "af_soul", "af_electra_flash", "af_rusty_sea-urchin",
        "af_dummy_kolobok", "lz_nlc_af_dummy_glassbeads",
        "lz_nlc_af_dummy_dummy", "af_dummy_pellicle",
    },
}
NATURAL = set().union(*NATURAL_CLASSES.values())
COOKED_BY_STAGE = {
    stage: {f"af_{family}_{stage}" for family in FAMILIES}
    for stage in range(1, 5)
}
COOKED = set().union(*COOKED_BY_STAGE.values())
COOKING_BASES = NATURAL_CLASSES[4]
OLD_WRONG_BASES = {"af_dummy_simbion", "lz_nlc_af_dummy_battery", "af_dummy_spring"}
ABSOLUTES = COOKED_BY_STAGE[4]
CLASS_COSTS = {1: 5000, 2: 8000, 3: 12000, 4: 10000}
STAGE_COSTS = {1: 18000, 2: 26000, 3: 38000, 4: 55000}
QUEST_ARTIFACTS = {"af_compass", "af_oasis_heart", "af_quest_b14_twisted"}


def fail(message: str, errors: list[str]) -> None:
    errors.append(message)


def load_table() -> configparser.ConfigParser:
    parser = configparser.ConfigParser(
        interpolation=None, delimiters=("=",), comment_prefixes=(";",),
        inline_comment_prefixes=(";",), strict=True,
    )
    parser.optionxform = str
    parser.read(TABLE, encoding="utf-8-sig")
    return parser


def csv(value: str) -> list[str]:
    return [part.strip() for part in value.split(",") if part.strip()]


def declared_sections() -> set[str]:
    result: set[str] = set()
    for path in (FULL_ARTIFACTS, NATURAL_ARTIFACTS):
        result.update(re.findall(r"(?m)^!?\[([^\]:]+)(?::[^\]]+)?\]", path.read_text(encoding="utf-8-sig")))
    return result


def artifact_costs() -> dict[str, int]:
    result: dict[str, int] = {}
    for path in (FULL_ARTIFACTS, NATURAL_ARTIFACTS):
        section = ""
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            header = re.match(r"^!?\[([^\]:]+)(?::[^\]]+)?\]", line)
            if header:
                section = header.group(1)
                continue
            cost = re.match(r"^\s*cost\s*=\s*([0-9]+)\s*$", line)
            if cost and section:
                result[section] = int(cost.group(1))
    return result


def parse_trade_section(path: Path, section_name: str) -> dict[str, str | None]:
    text = path.read_text(encoding="utf-8-sig")
    section = re.search(
        rf"(?ms)^\[{re.escape(section_name)}\]\s*(.*?)(?=^\[|\Z)", text
    )
    if not section:
        raise ValueError(f"{path.name}: missing [{section_name}]")
    entries: dict[str, str | None] = {}
    for raw_line in section.group(1).splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        match = re.fullmatch(r"([a-zA-Z0-9_.-]+)(?:\s*=\s*(.*))?", line)
        if not match:
            continue
        item, value = match.groups()
        if item in entries:
            raise ValueError(f"{path.name}: duplicate {item} in [{section_name}]")
        entries[item] = value.strip() if value is not None else None
    return entries


def validate_table(parser: configparser.ConfigParser, errors: list[str]) -> None:
    zones = parser["spawn_zones"]
    ids = sorted(int(value) for value in zones)
    if ids != list(range(1, 42)):
        fail(f"spawn_zones must contain IDs 01..41, got {ids}", errors)
    counts = Counter(zones[key].strip() for key in zones)
    if dict(counts) != EXPECTED_ZONE_COUNTS:
        fail(f"zone family counts differ: {dict(counts)}", errors)

    declared = declared_sections()
    seen = {artifact_class: set() for artifact_class in range(1, 5)}
    for family in sorted(ZONE_FAMILIES):
        section = parser[f"spawn_family_{family}"]
        for artifact_class in range(1, 5):
            artifacts = csv(section.get(f"class_{artifact_class}", ""))
            weights = csv(section.get(f"class_{artifact_class}_weights", ""))
            if len(artifacts) != len(weights):
                fail(f"{family} class_{artifact_class}: section/weight count differs", errors)
            for weight in weights:
                if float(weight) <= 0:
                    fail(f"{family} class_{artifact_class}: non-positive weight", errors)
            for artifact in artifacts:
                if artifact not in declared:
                    fail(f"undefined natural artifact {artifact}", errors)
                if artifact in seen[artifact_class]:
                    fail(f"duplicate class {artifact_class} artifact {artifact}", errors)
                seen[artifact_class].add(artifact)
                if artifact in COOKED:
                    fail(f"non-natural artifact present in spawn: {artifact}", errors)

    if seen != NATURAL_CLASSES:
        for artifact_class in range(1, 5):
            missing = sorted(NATURAL_CLASSES[artifact_class] - seen[artifact_class])
            extra = sorted(seen[artifact_class] - NATURAL_CLASSES[artifact_class])
            if missing or extra:
                fail(f"class {artifact_class}: missing={missing}, extra={extra}", errors)

    psi_class_3 = set(csv(parser["spawn_family_psi"].get("class_3", "")))
    electric_class_3 = set(csv(parser["spawn_family_electric"].get("class_3", "")))
    if "af_dummy_simbion" not in psi_class_3:
        fail("Symbion must spawn as a class-III psi artifact", errors)
    if "lz_nlc_af_dummy_battery" not in electric_class_3:
        fail("Battery must spawn as a class-III electrical artifact", errors)
    for family in ZONE_FAMILIES - {"psi"}:
        if "af_dummy_simbion" in set(csv(parser[f"spawn_family_{family}"].get("class_3", ""))):
            fail(f"Symbion is assigned to non-psi family {family}", errors)
    for family in ZONE_FAMILIES - {"electric"}:
        if "lz_nlc_af_dummy_battery" in set(csv(parser[f"spawn_family_{family}"].get("class_3", ""))):
            fail(f"Battery is assigned to non-electric family {family}", errors)

    chances = tuple(
        parser["spawn_class_chances"].getint(f"class_{value}")
        for value in range(1, 5)
    )
    expected_chances = (30, 20, 20, 30)
    if chances != expected_chances or sum(chances) != 100 or chances[0] != chances[3]:
        fail(f"global class chances: got {chances}, expected {expected_chances}", errors)

    expected_phases = {
        "spawn_phase_0_4": (0, 4, 18),
        "spawn_phase_5_9": (5, 9, 22),
        "spawn_phase_10_plus": (10, -1, 24),
    }
    for phase, expected in expected_phases.items():
        values = parser[phase]
        actual = (
            int(values["day_from"]), int(values["day_to"]),
            int(values["total_cap"]),
        )
        if actual != expected:
            fail(f"{phase}: got {actual}, expected {expected}", errors)
        extra = set(values) - {"day_from", "day_to", "total_cap"}
        if extra:
            fail(f"{phase}: only total cap may vary by day; extra={sorted(extra)}", errors)

    settings = parser["spawn_settings"]
    expected_settings = {
        "initial_artifacts": 10, "initial_minimum_per_class": 1,
        "artifacts_per_emission": 3,
        "empty_zone_cooldown_emissions": 1,
    }
    for key, expected in expected_settings.items():
        if int(settings[key]) != expected:
            fail(f"{key}: expected {expected}", errors)

    spawn_config = SPAWN_CONFIG.read_text(encoding="utf-8-sig")
    spawn_runtime = SPAWN_RUNTIME.read_text(encoding="utf-8-sig")
    for retired in ("class_3_global_cap", "class_4_global_cap", "add_per_emission"):
        if retired in settings or retired in spawn_config or retired in spawn_runtime:
            fail(f"retired day/class limiter remains: {retired}", errors)
    if re.search(r"class_[1-4]_chance", spawn_config + spawn_runtime):
        fail("class chances are still read from a day phase", errors)
    if "tier_chances" in spawn_config or "tier_2_global_cap" in spawn_config:
        fail("spawn config still contains retired tier semantics", errors)
    for needle in (
        "config.artifact_class(section)", "entry.tier = nil",
        "alife():release(object, true)", "class_4 = counts[4]",
        "local chances = config.class_chances()",
        "settings.artifacts_per_emission",
        "settings.initial_minimum_per_class - counts[artifact_class]",
        "data, initial_phase, required, artifact_class",
        "initial_coverage_ready(counts)",
    ):
        if needle not in spawn_runtime:
            fail(f"spawn runtime is missing {needle!r}", errors)


def validate_cooking(errors: list[str]) -> None:
    registry = REGISTRY.read_text(encoding="utf-8-sig")
    runtime = COOKING_RUNTIME.read_text(encoding="utf-8-sig")
    accept_radius = re.search(
        r"(?m)^\s*cooking_accept_radius\s*=\s*([0-9.]+)\s*,", runtime,
    )
    if not accept_radius or float(accept_radius.group(1)) != 5.0:
        fail("cooking acceptance radius must remain 5 metres", errors)
    registered_prefix_roles: dict[str, str | None] = {}
    registered_prefix_names: dict[str, str | None] = {}
    for block in re.findall(r"(?ms)^\s*\{\s*(.*?)^\s*\},?\s*$", registry):
        prefix = re.search(r'prefix\s*=\s*"([^"]+)"', block)
        if not prefix:
            continue
        role = re.search(r'role\s*=\s*"([^"]+)"', block)
        registered_prefix_roles[prefix.group(1)] = role.group(1) if role else None
        name = re.search(r'name\s*=\s*"([^"]+)"', block)
        registered_prefix_names[prefix.group(1)] = name.group(1) if name else None
    for prefix, expected_role in COOKING_PREFIX_ROLES.items():
        actual_role = registered_prefix_roles.get(prefix)
        if actual_role != expected_role:
            fail(
                f"cooking anomaly prefix {prefix}: role={actual_role}, "
                f"expected={expected_role}", errors,
            )
    for prefix, expected_name in GRAVITY_LORE_NAMES.items():
        actual_name = registered_prefix_names.get(prefix)
        if actual_name != expected_name:
            fail(
                f"gravitational anomaly prefix {prefix}: name={actual_name}, "
                f"expected={expected_name}", errors,
            )

    aliases = dict(re.findall(
        r'(?m)^\s*(pf_proc_[a-z_]+)\s*=\s*"([^"]+)"\s*,?$', registry,
    ))
    for alias, expected_role in PROCEDURAL_COOKING_ROLES.items():
        target = aliases.get(alias)
        matching_prefixes = [
            prefix for prefix in COOKING_PREFIX_ROLES
            if target and target.startswith(prefix)
        ]
        actual_role = (
            COOKING_PREFIX_ROLES[max(matching_prefixes, key=len)]
            if matching_prefixes else None
        )
        if actual_role != expected_role:
            fail(
                f"procedural anomaly {alias}: target={target}, role={actual_role}, "
                f"expected={expected_role}", errors,
            )
    if re.search(r"pf_proc_[a-z_]+\s*=\s*\{", registry):
        fail("procedural anomaly aliases are still display-only tables", errors)

    chain_pattern = re.compile(
        r'add_chain\("([^"]+)",\s*"([^"]+)",\s*\{([^}]+)\}\)'
    )
    chains = {
        family: (base, tuple(re.findall(r'"([^"]+)"', raw_roles)))
        for base, family, raw_roles in chain_pattern.findall(registry)
    }
    expected_bases = {
        "spirit": "af_soul", "cry": "af_electra_flash",
        "dik": "af_rusty_sea-urchin", "kol": "af_dummy_kolobok",
        "babka": "lz_nlc_af_dummy_glassbeads",
        "pudd": "lz_nlc_af_dummy_dummy", "armor": "af_dummy_pellicle",
    }
    if {family: value[0] for family, value in chains.items()} != expected_bases:
        fail("cooking base registry differs from the approved seven class-IV bases", errors)
    for old_base in OLD_WRONG_BASES:
        if re.search(rf'add_chain\("{re.escape(old_base)}"', registry):
            fail(f"retired cooking base still registered: {old_base}", errors)

    for family, roles in COOKING_ROLES.items():
        if not chains.get(family) or chains[family][1] != roles:
            fail(f"{family}: cooking anomaly sequence differs", errors)

    costs = artifact_costs()
    for artifact_class, artifacts in NATURAL_CLASSES.items():
        for artifact in artifacts:
            if costs.get(artifact) != CLASS_COSTS[artifact_class]:
                fail(f"{artifact}: cost {costs.get(artifact)}, expected {CLASS_COSTS[artifact_class]}", errors)
    for stage, artifacts in COOKED_BY_STAGE.items():
        for artifact in artifacts:
            if costs.get(artifact) != STAGE_COSTS[stage]:
                fail(f"{artifact}: cooked cost changed to {costs.get(artifact)}", errors)
    if costs.get("af_dummy_buliz") != 500:
        fail("Boulder cost must remain 500", errors)


def validate_texts(parser: configparser.ConfigParser, errors: list[str]) -> None:
    expected_minutes = {
        stage: int(parser["cooking_times"][f"stage_{stage}_minutes"])
        for stage in range(1, 5)
    }
    expected_sources = {
        "rus": {
            "spirit": "«Душу»", "cry": "«Вспышку»", "dik": "«Морской ёж»",
        },
        "eng": {
            "spirit": "Soul", "cry": "Flash", "dik": "Sea Urchin",
        },
    }
    forbidden = {
        "rus": ("«Симбион»", "«Батарейка»", "«Пружина»"),
        "eng": ("Symbiont", "Battery", "Spring"),
    }
    role_labels = {
        "rus": {
            "zharka": "«Жарка»", "galant": "«Электра»",
            "buzz": "«Холодец»", "mosquito_bald": "«Трамплин»",
            "mincer": "«Карусель»",
        },
        "eng": {
            "zharka": "Burner", "galant": "Electra",
            "buzz": "Cold Jelly", "mosquito_bald": "Springboard",
            "mincer": "Whirligig",
        },
    }
    anomaly_labels = {
        "rus": {
            "st_lz_nlc_anomaly_springboard": "Трамплин",
            "st_lz_nlc_anomaly_vortex": "Воронка",
            "st_lz_nlc_anomaly_whirligig": "Карусель",
        },
        "eng": {
            "st_lz_nlc_anomaly_springboard": "Springboard",
            "st_lz_nlc_anomaly_vortex": "Vortex",
            "st_lz_nlc_anomaly_whirligig": "Whirligig",
        },
    }
    for path, language in ((RUS_TEXT, "rus"), (ENG_TEXT, "eng")):
        root = ElementTree.parse(path).getroot()
        by_id = {
            node.attrib.get("id", ""): "".join(node.itertext())
            for node in root.findall("string")
        }
        for text_id, expected in anomaly_labels[language].items():
            if by_id.get(text_id) != expected:
                fail(
                    f"{language}: {text_id}={by_id.get(text_id)!r}, "
                    f"expected={expected!r}", errors,
                )
        for retired in ("Мясорубка", "Mincer", "Meat Grinder"):
            if any(retired in text for text in by_id.values()):
                fail(f"{language}: retired anomaly label remains: {retired}", errors)
        counts = Counter()
        for text_id, text in by_id.items():
            match = re.fullmatch(r"st_lz_nlc_enc_recipe_[a-z]+_([1-4])_text", text_id)
            if not match:
                continue
            stage = int(match.group(1))
            needle = (
                f"Время варки — {expected_minutes[stage]} игровых минут."
                if language == "rus"
                else f"Cooking takes {expected_minutes[stage]} in-game minutes."
            )
            if needle not in text:
                fail(f"{language}: {text_id} has wrong cooking time", errors)
            counts[stage] += 1
        if counts != Counter({1: 7, 2: 7, 3: 7, 4: 7}):
            fail(f"{language}: expected 28 recipe articles, got {dict(counts)}", errors)

        for family, roles in COOKING_ROLES.items():
            for stage, role in enumerate(roles, 1):
                text_id = f"st_lz_nlc_enc_recipe_{family}_{stage}_text"
                text = by_id.get(text_id, "")
                label = re.escape(role_labels[language][role])
                prefix = "аномалию" if language == "rus" else "into"
                if not re.search(
                    rf"{prefix}\s+%c\[[^]]+\]{label}%c\[default\]", text
                ):
                    fail(f"{language}: {text_id} disagrees with runtime anomaly {role}", errors)

        for family, source in expected_sources[language].items():
            text_id = f"st_lz_nlc_enc_recipe_{family}_1_text"
            text = by_id.get(text_id, "")
            if source not in text:
                fail(f"{language}: {text_id} misses source {source}", errors)
            for retired in forbidden[language]:
                if retired in text:
                    fail(f"{language}: {text_id} still mentions retired source {retired}", errors)
        intro = by_id.get("pf_hermann_recipe_intro_hermann", "")
        if ("обычного артефакта" in intro or "ordinary artefact" in intro):
            fail(f"{language}: Hermann intro still uses the retired base wording", errors)
        purchase = by_id.get("pf_hermann_recipe_purchase_actor", "")
        if ("артпреобразования" in purchase or "artefact-transformation" in purchase):
            fail(f"{language}: Hermann still calls modification transformation", errors)


def validate_artifact_tooltip_spacing(errors: list[str]) -> None:
    text_files = (
        (RUS_TEXT, "rus/cooking"),
        (ENG_TEXT, "eng/cooking"),
        (RUS_NATURAL_TEXT, "rus/natural"),
        (ENG_NATURAL_TEXT, "eng/natural"),
    )
    checked = 0
    for path, label in text_files:
        root = ElementTree.parse(path).getroot()
        for node in root.findall("string"):
            text_id = node.attrib.get("id", "")
            text = "".join(node.itertext())
            if (
                not text_id.startswith("st_lz_nlc_af_")
                or not text_id.endswith("_descr")
                or "%c[" not in text
            ):
                continue
            checked += 1
            if r"\n\n%c[" not in text:
                fail(
                    f"{label}: {text_id} has no visible blank line before its class/stage label",
                    errors,
                )
            if "\n" in text or "\r" in text:
                fail(
                    f"{label}: {text_id} relies on a physical XML line break",
                    errors,
                )
    if checked != 88:
        fail(f"expected 88 localized artifact tooltip descriptions, got {checked}", errors)


def validate_recipe_sale(errors: list[str]) -> None:
    script = HERMANN_RECIPES.read_text(encoding="utf-8-sig")
    recipe_infos = dict(re.findall(
        r'(?m)^\s*([a-z]+_[1-4])\s*=\s*\{[^\n]*\binfo\s*=\s*"([^"]+)"',
        script,
    ))
    recipe_entries = set(recipe_infos)
    expected = {
        f"{family}_{stage}" for family in FAMILIES for stage in range(1, 5)
    }
    if recipe_entries != expected:
        fail(f"Hermann recipe catalogue differs: missing={sorted(expected - recipe_entries)}", errors)
    expected_infos = {
        recipe_id: f"lz_nlc_recipe_{recipe_id}_known" for recipe_id in expected
    }
    if recipe_infos != expected_infos:
        fail("Hermann recipe identifiers were deleted or renamed", errors)
    dialog_text = DIALOGS.read_text(encoding="utf-8-sig")
    selected = set(re.findall(
        r"select_recipe_([a-z]+_[1-4])</action>", dialog_text
    ))
    if selected != expected:
        fail(f"Hermann dialog recipe actions differ: missing={sorted(expected - selected)}", errors)


def validate_trade(errors: list[str]) -> None:
    try:
        hermann = parse_trade_section(HERMANN_TRADE, "trade_pf_bunker_hermann_buy")
        hawaiian = parse_trade_section(HAWAIIAN_TRADE, "trade_pf_yanov_hawaiian_buy")
    except ValueError as exc:
        fail(str(exc), errors)
        return

    expected_trade = NATURAL | COOKED | {"af_dummy_buliz"}
    for artifact in sorted(expected_trade):
        if hermann.get(artifact) != "0.60, 0.60":
            fail(f"Hermann buyback missing/wrong for {artifact}", errors)
        if hawaiian.get(artifact) != "0.45, 0.45":
            fail(f"Hawaiian buyback missing/wrong for {artifact}", errors)
    for artifact in QUEST_ARTIFACTS:
        if hermann.get(artifact, "missing") is not None:
            fail(f"Hermann must deny quest artifact {artifact}", errors)

    beard = BEARD_TRADE.read_text(encoding="utf-8-sig")
    for line_number, line in enumerate(beard.splitlines(), 1):
        match = re.match(r"^\s*([a-zA-Z0-9_.-]+)\s*=", line)
        if match and match.group(1) in expected_trade:
            fail(f"Beard artifact trade re-enabled at line {line_number}", errors)


def validate_taskboard(errors: list[str]) -> None:
    parser = configparser.ConfigParser(
        interpolation=None, delimiters=("=",), comment_prefixes=(";",),
        inline_comment_prefixes=(";",), strict=True,
    )
    parser.optionxform = str
    parser.read(TASKBOARD_ORDERS, encoding="utf-8-sig")

    ordinary_ids = {
        "lz_order_common_artefacts",
        "lz_order_thermal_artefacts",
        "lz_order_three_candles",
    }
    ordinary: list[str] = []
    for order_id in ordinary_ids:
        section = parser[order_id]
        ordinary.extend(csv(section.get("item_pool", "")))
        if section.get("recipe_gated", "false").lower() == "true":
            fail(f"ordinary Taskboard pool is recipe-gated: {order_id}", errors)
        if section.get("reward_from_item_cost", "false").lower() != "true":
            fail(f"ordinary Taskboard pool lacks dynamic reward: {order_id}", errors)
        if section.getint("reward_percent_of_cost", fallback=0) != 72:
            fail(f"ordinary Taskboard pool is not 20% above Hermann: {order_id}", errors)
    ordinary_counts = Counter(ordinary)
    expected_ordinary = NATURAL
    if set(ordinary) != expected_ordinary or any(count != 1 for count in ordinary_counts.values()):
        fail(
            "ordinary Taskboard catalogue differs: "
            f"missing={sorted(expected_ordinary - set(ordinary))}, "
            f"extra={sorted(set(ordinary) - expected_ordinary)}, "
            f"duplicates={sorted(item for item, count in ordinary_counts.items() if count != 1)}",
            errors,
        )

    cooked: list[str] = []
    for section_name in parser.sections():
        section = parser[section_name]
        if section.get("recipe_gated", "false").lower() == "true":
            cooked.extend(csv(section.get("item_pool", "")))
    expected_cooked = COOKED_BY_STAGE[1] | COOKED_BY_STAGE[2] | COOKED_BY_STAGE[3]
    if set(cooked) != expected_cooked:
        fail(
            "cooked Taskboard catalogue differs: "
            f"missing={sorted(expected_cooked - set(cooked))}, "
            f"extra={sorted(set(cooked) - expected_cooked)}",
            errors,
        )
    forbidden = set(ordinary) | set(cooked)
    if "af_dummy_buliz" in forbidden or forbidden & ABSOLUTES:
        fail("Taskboard catalogue contains Boulder or an Absolute", errors)

    script = TASKBOARD_SCRIPT.read_text(encoding="utf-8-sig")
    reward_block = re.search(
        r"(?ms)^local minimum_reward_values\s*=\s*\{(.*?)^\}", script
    )
    rewards: dict[str, int] = {}
    for match in re.finditer(
        r'(?m)^\s*(?:([a-zA-Z0-9_.-]+)|\["([^"]+)"\])\s*=\s*([0-9]+)',
        reward_block.group(1) if reward_block else "",
    ):
        rewards[match.group(1) or match.group(2)] = int(match.group(3))
    expected_rewards = {
        artifact: int(CLASS_COSTS[artifact_class] * 0.60 * 1.20)
        for artifact_class, artifacts in NATURAL_CLASSES.items()
        for artifact in artifacts
    }
    for artifact, expected in expected_rewards.items():
        if rewards.get(artifact) != expected:
            fail(
                f"Taskboard reward floor for {artifact}: "
                f"{rewards.get(artifact)} != {expected}", errors,
            )
    for token in (
        'lowered == "af_dummy_buliz"',
        "lz_nlc_anomaly_registry.is_absolute(section)",
        "reward_from_item_cost",
        "reward_percent_of_cost",
    ):
        if token not in script:
            fail(f"Taskboard script misses artifact guard token: {token}", errors)


def validate_guardian(errors: list[str]) -> None:
    registry = REGISTRY.read_text(encoding="utf-8-sig")
    block = re.search(r"(?ms)\[5\]\s*=\s*\{(.*?)\n\s*\}", registry)
    registered = set(re.findall(r'"(af_[a-z]+_4)"', block.group(1))) if block else set()
    if registered != ABSOLUTES:
        fail(f"absolute registry differs: {sorted(registered)}", errors)

    guardian = GUARDIAN.read_text(encoding="utf-8-sig")
    requirements_block = re.search(
        r"(?ms)^local COOKED_EXCHANGE_REQUIREMENTS\s*=\s*\{(.*?)^\}", guardian,
    )
    requirements = {
        int(tier): int(amount)
        for tier, amount in re.findall(
            r"\[([0-9]+)\]\s*=\s*([0-9]+)",
            requirements_block.group(1) if requirements_block else "",
        )
    }
    if requirements != {2: 5, 3: 3, 4: 2}:
        fail(f"Guardian cooked exchange ladder differs: {requirements}", errors)

    for function_name in (
        "is_exchangeable_cooked", "rucksack_cooked_artifacts",
        "can_exchange_cooked_tier", "exchange_cooked_tier",
        "can_exchange_absolute", "exchange_absolute",
    ):
        if f"function {function_name}" not in guardian and f"local function {function_name}" not in guardian:
            fail(f"Guardian missing {function_name}", errors)
    for token in (
        "db.actor:is_on_belt(item)",
        "lz_nlc_anomaly_registry.artifact_tier(item:section())",
        "tier >= 2 and tier <= 5",
        "left:id() < right:id()",
        "#artifacts < amount",
    ):
        if token not in guardian:
            fail(f"Guardian cooked exchange misses guard token: {token}", errors)
    if guardian.count("lz_nlc_anomaly_registry.is_absolute") < 2:
        fail("Guardian Absolute precondition/action are not both registry-guarded", errors)

    transaction_start = guardian.find("local function exchange_artifacts_for_hint")
    grant_position = guardian.find("pf_dynamic_stashes.grant_hint", transaction_start)
    transfer_position = guardian.find("db.actor:transfer_item", transaction_start)
    if min(transaction_start, grant_position, transfer_position) < 0:
        fail("Guardian cooked exchange transaction is incomplete", errors)
    elif grant_position > transfer_position:
        fail("Guardian removes artifacts before granting the red stash hint", errors)

    facade = COOKING_FACADE.read_text(encoding="utf-8-sig")
    mapped = set(re.findall(
        r'exchange_absolute\(first_speaker, second_speaker, "(af_[a-z]+_4)"\)',
        facade,
    ))
    if mapped != ABSOLUTES:
        fail(f"Guardian facade exposes non-exact Absolute list: {sorted(mapped)}", errors)
    bundle_mappings = dict(re.findall(
        r"function guardian_exchange_([a-z]+)_bundle\(.*?"
        r"exchange_cooked_tier\(first_speaker, second_speaker, ([234])\)",
        facade, re.DOTALL,
    ))
    bundle_preconditions = dict(re.findall(
        r"function guardian_can_([a-z]+)_bundle\(.*?"
        r"can_exchange_cooked_tier\(([234])\)",
        facade, re.DOTALL,
    ))
    expected_bundle_mappings = {
        "modificat": "2", "mesomodificat": "3", "hypermodificat": "4",
    }
    if bundle_mappings != expected_bundle_mappings:
        fail(f"Guardian facade bundle mappings differ: {bundle_mappings}", errors)
    if bundle_preconditions != expected_bundle_mappings:
        fail(f"Guardian facade bundle preconditions differ: {bundle_preconditions}", errors)

    root = ElementTree.parse(DIALOGS).getroot()
    dialog = root.find("./dialog[@id='pf_monolith_guardian_dialog']")
    if dialog is None:
        fail("Guardian dialog is missing", errors)
        return
    phrases = {
        node.attrib["id"]: node
        for node in dialog.findall("./phrase_list/phrase")
    }
    if len(phrases) != len(dialog.findall("./phrase_list/phrase")):
        fail("Guardian dialog contains duplicate phrase IDs", errors)
    offer = phrases.get("61")
    offer_next = [node.text for node in offer.findall("next")] if offer is not None else []
    expected_next = ["66", "67", "68", "70", "71", "72", "73", "74", "75", "76", "64"]
    if offer_next != expected_next:
        fail(f"Guardian flat exchange branches differ: {offer_next}", errors)

    bundle_phrases = {
        "66": "modificat", "67": "mesomodificat", "68": "hypermodificat",
    }
    for phrase_id, stem in bundle_phrases.items():
        phrase = phrases.get(phrase_id)
        if phrase is None:
            fail(f"Guardian bundle phrase {phrase_id} is missing", errors)
            continue
        if phrase.findtext("precondition") != f"lz_nlc_art_cooking.guardian_can_{stem}_bundle":
            fail(f"Guardian bundle phrase {phrase_id} has wrong precondition", errors)
        if phrase.findtext("action") != f"lz_nlc_art_cooking.guardian_exchange_{stem}_bundle":
            fail(f"Guardian bundle phrase {phrase_id} has wrong action", errors)

    for offset, family in enumerate(FAMILIES):
        phrase_id = str(70 + offset)
        phrase = phrases.get(phrase_id)
        if phrase is None:
            fail(f"Guardian Absolute phrase {phrase_id} is missing", errors)
            continue
        if phrase.findtext("precondition") != f"lz_nlc_art_cooking.guardian_can_absolute_{family}":
            fail(f"Guardian Absolute phrase {phrase_id} has wrong precondition", errors)
        if phrase.findtext("action") != f"lz_nlc_art_cooking.guardian_exchange_absolute_{family}":
            fail(f"Guardian Absolute phrase {phrase_id} has wrong action", errors)

    guardian_text_ids = {
        phrase.findtext("text") for phrase in phrases.values()
        if (phrase.findtext("text") or "").startswith("st_lz_nlc_guardian_")
    }
    for path, language in ((RUS_TEXT, "rus"), (ENG_TEXT, "eng")):
        localized = {
            node.attrib.get("id", "")
            for node in ElementTree.parse(path).getroot().findall("string")
        }
        missing = guardian_text_ids - localized
        if missing:
            fail(f"{language}: Guardian dialog text IDs missing: {sorted(missing)}", errors)


def validate_world_population(errors: list[str]) -> None:
    field_paths = sorted(FIELD_ROOT.glob("pf_art_field_*.ltx"))
    if len(field_paths) != 5:
        fail(f"expected five legacy artifact fields, got {len(field_paths)}", errors)
    for path in field_paths:
        text = path.read_text(encoding="utf-8-sig")
        if not re.search(r"(?m)^respawn_tries\s*=\s*0\s*$", text):
            fail(f"{path.name}: respawn_tries is not zero", errors)
        if not re.search(r"(?m)^max_artefacts\s*=\s*0\s*$", text):
            fail(f"{path.name}: max_artefacts is not zero", errors)
        if re.search(r"(?m)^artefacts[ \t]*=[ \t]*\S", text):
            fail(f"{path.name}: legacy artifact pool is populated", errors)

    native_binder = NATIVE_BINDER.read_text(encoding="utf-8-sig")
    for needle in (
        'self.respawn_artefacts = level.name() ~= "pripyat_full"',
        'if level.name() == "pripyat_full" then',
        "self.respawn_artefacts = false",
    ):
        if needle not in native_binder:
            fail(f"native surge spawn guard is missing {needle!r}", errors)

    if (not ANOMALY_PROTOTYPE.is_file() or not ANOMALY_ZONES.is_file()
            or not ANOMALY_SERVER.is_file()):
        fail("procedural anomaly addon required by natural spawn is absent", errors)
        return
    prototype = json.loads(ANOMALY_PROTOTYPE.read_text(encoding="utf-8-sig"))
    layouts = prototype.get("layouts", [])
    if prototype.get("status") != "passed" or len(layouts) != 1:
        fail("procedural anomaly static verification is not passed", errors)
    else:
        layout = layouts[0]
        if (layout.get("groups"), layout.get("clustered"), layout.get("solitary")) != (41, 612, 600):
            fail("procedural anomaly layout must stay 41/612/600", errors)
    zone_text = ANOMALY_ZONES.read_text(encoding="utf-8-sig")
    point_sections = set(re.findall(
        r"(?m)^\[(pf_proc_(?!aura_)[a-z_]+)\]", zone_text,
    ))
    point_sections = {section for section in point_sections if "_psi_" not in section}
    if point_sections != set(PROCEDURAL_COOKING_ROLES):
        fail(
            "procedural non-psi point sections differ from cooking aliases: "
            f"missing={sorted(point_sections - set(PROCEDURAL_COOKING_ROLES))}, "
            f"stale={sorted(set(PROCEDURAL_COOKING_ROLES) - point_sections)}",
            errors,
        )
    for match in re.finditer(r"(?m)^artefact_spawn_(?:rnd|count)\s*=\s*([^;\s]+)", zone_text):
        if float(match.group(1)) != 0:
            fail(f"procedural native spawn is active: {match.group(0)}", errors)

    server_text = ANOMALY_SERVER.read_text(encoding="utf-8-sig")
    update_blocks = re.findall(
        r"(?ms)^function se_zone_(?:anom|torrid|visual):update\(\).*?^end$",
        server_text,
    )
    if len(update_blocks) != 3:
        fail(f"expected three procedural server update guards, got {len(update_blocks)}", errors)
    for block in update_blocks:
        if not re.search(r"if self\.pf_is_procedural then(?:\s+return|\s*\n\s*return)", block):
            fail("procedural server update can reach native spawn_artefacts", errors)


def main() -> int:
    errors: list[str] = []
    parser = load_table()
    validate_table(parser, errors)
    validate_cooking(errors)
    validate_texts(parser, errors)
    validate_artifact_tooltip_spacing(errors)
    validate_recipe_sale(errors)
    validate_trade(errors)
    validate_taskboard(errors)
    validate_guardian(errors)
    validate_world_population(errors)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        print(f"FAILED: {len(errors)} issue(s)")
        return 1
    print("OK: natural spawn contains exactly 28 class I-IV artifacts (7/7/7/7)")
    print("OK: class chances stay 30/20/20/30 from day zero; only total cap changes")
    print("OK: new-game population guarantees at least one artifact of every class I-IV")
    print("OK: seven class-IV bases and first cooking anomalies match the approved scheme")
    print("OK: native, grouped and all 600 solitary anomaly points keep cooking roles at every strength")
    print("OK: all 28 encyclopedia recipes match runtime anomaly sequences and times")
    print("OK: all 44 artifact tooltips per language keep a visible paragraph break")
    print("OK: all 28 cooked sections keep the 18/26/38/55k price ladder")
    print("OK: Hermann preserves all 28 recipe IDs and corrects their existing content")
    print("OK: Hermann and Hawaiian buy every natural/cooked artifact")
    print("OK: Taskboard covers 28 natural and 21 cooked non-Absolutes")
    print("OK: natural contracts pay 20% above Hermann; Boulder/Absolutes excluded")
    print("OK: Guardian exchanges exact 5/3/2/1 cooked bundles with belt protection")
    print("OK: legacy, surge-binder and procedural native spawn paths remain disabled")
    return 0


if __name__ == "__main__":
    sys.exit(main())

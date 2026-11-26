# Localization guide

Chronicles of Pripyat ships Russian (`rus`), English (`eng`), and French (`fra`). French is bundled as a separate IX-Ray addon at `ixr_addons/~pf-french`. New languages can use the same addon layout. Use a current build for the latest string IDs. UTF-8 localization support shipped with `2026.11.22-dev.116`. [Dev release notes](https://github.com/hardcoon/chronicles-of-pripyat-ixray/releases/tag/dev-2026.08.26.1)

## French addon reference

| Path inside `ixr_addons/~pf-french` | Purpose |
| --- | --- |
| `addon.init` | Enables the addon and provides its metadata. |
| `configs/localization.ltx` | Registers `rus,eng,fra` in the `[string_table]` section. |
| `configs/text/fra/` | French string tables and XMLOverride fragments. |
| `configs/text/eng/st_pf_french_fallback.xml` | Additional English fallback strings. |
| `fonts/fra/` | French font files, including `default.ttf`. |
| `configs/ui/textures_descr/ui_pf_french_flag.xml` and `textures/ui/ui_pf_french_flag.dds` | Assets for the French shortcut in the main menu. |
| `ATTRIBUTION.txt` | Translation credits. |

[Browse the French addon](ixr_addons/~pf-french)

## Prepare a language addon

### 1. Choose a locale and create the folders

Use a short lowercase locale ID. Use it consistently in the language list, the text directory, and the font directory. For Chinese, `zh-hans` and `zh-hant` can be registered as separate locales for Simplified and Traditional Chinese.

Here is a minimal layout for a Simplified Chinese addon.

```text
ixr_addons/
  ~pf-zh-hans/
    addon.init
    configs/
      mod_system_pf_zh_hans.ltx
      text/
        zh-hans/
          st_pf_zh_hans.xml
    fonts/
      zh-hans/
        default.ttf
```

Save `addon.init` at the root of the addon. Replace the sample author name before sharing the pack.

```text
name: Simplified Chinese localization
author: Translator name
version: 1.0
description: Simplified Chinese text and fonts for Chronicles of Pripyat.
```

[IX-Ray addon documentation](https://ixray-team.github.io/ixray-1.6-stcop/en/addon-system/general-information.html)

### 2. Translate the XML string tables

Start with the English files from the installed game and its enabled addons. Use `zz_pf_field_archive.xml` for the project's custom main menu and `ui_st_mm.xml` for standard menu strings. The inventory below also lists item descriptions, dialogues, quests, and PDA files. If a file exists in several layers, work from the version loaded by the game and check for related XMLOverride fragments.

- Keep every `<string id="...">` value. Translate the text inside `<text>`. Use each ID once in the final tables for your locale.
- Preserve placeholders such as `%s`, `%d`, and `$$ACTION_...$$`, along with markup such as `%c[...]` and any `\n` sequences used for line breaks. Escape XML characters such as `&` as `&amp;`.
- Save new tables as UTF-8 and set the XML declaration to `encoding="UTF-8"`. Use ordinary table names such as `st_pf_zh_hans.xml`. A `mod_*.xml` file is an XMLOverride fragment and is loaded through its matching base XML.
- Translate as much as you have ready. Missing and empty entries use the English entry for the same ID when it exists.

This example translates the custom **New Game** label and names the locale in the language selector. For a pack that includes both Chinese locales, give each locale its own tables.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<string_table>
  <string id="pf_fa_new_game">
    <text>新游戏</text>
  </string>
  <string id="zh-hans">
    <text>简体中文</text>
  </string>
</string_table>
```

[IX-Ray localization documentation](https://ixray-team.github.io/ixray-1.6-stcop/en/configs/localization.html) · [IX-Ray XMLOverride documentation](https://ixray-team.github.io/ixray-1.6-stcop/en/configs/xml-override.html)

### 3. Add the font files

Put a TTF with a Unicode character map in `fonts/zh-hans/default.ttf`. The game tries the font family named in `fonts.ltx` inside the selected locale's font directory and uses that locale's `default.ttf` when the requested file is absent. Include all characters used by the translation, plus Latin letters, digits, and punctuation used by English fallback text, controls, and numbers.

Both Chinese writing systems work with the game's horizontal left-to-right UI. The shipped font atlas size is `4096 × 4096`. The engine loads the font's glyphs into the atlas when it initializes a UI font. A large CJK font can exceed the limit at larger font sizes and resolutions. If it does, use a TTF subset with the characters your pack needs and test the result in-game. Include the font's redistribution license with the pack.

[IX-Ray font documentation](https://ixray-team.github.io/ixray-1.6-stcop/en/interface/fonts.html)

### 4. Register and test the language

French supplies the base language list through `configs/localization.ltx`. In your own addon, add this DLTX patch as `configs/mod_system_pf_zh_hans.ltx`. It appends `zh-hans` to the current `languages` list.

```ini
![string_table]
>languages = zh-hans
```

If you want a translated name in the language list while English is selected, define the `zh-hans` ID in a uniquely named English string table in your addon, with a value such as `Simplified Chinese`.

1. Install the addon folder directly under the game's `ixr_addons` directory and fully restart the game. Select the new entry under **Language** in the game options or enter `language zh-hans` in the console. The main-menu flag shortcuts use separate UI assets and callbacks.
2. Check the main menu, inventory, dialogue, and PDA at the resolutions you plan to support. Look for missing glyphs, clipped text, poor line breaks, and incorrect substitutions in strings with placeholders.
3. Check one translated string and one English fallback string. Switch between languages and restart the game to check that the selection is saved. Include the tested game build and translation credits when you share the addon.

[IX-Ray DLTX documentation](https://ixray-team.github.io/ixray-1.6-stcop/en/configs/dltx.html)

## Translation files in 2026.11.26-dev.126

The lists below give the installed XML paths in the release manifest for the three selectable languages. They include the base game and bundled addons, with XMLOverride fragments listed at their installed paths. The same filename can appear in more than one layer. Translation coverage is determined by the string IDs that the game loads.

The French addon accounts for 201 of the 236 French XML paths. Use the English list as the source inventory for a new language. Base game files are available in the launcher installation. [Game installation instructions](README.md)

<details>
<summary>French (fra) · 236 XML paths</summary>

```text
gamedata/configs/text/fra/st_achievement.xml
gamedata/configs/text/fra/st_characters.xml
gamedata/configs/text/fra/st_dialog_manager.xml
gamedata/configs/text/fra/st_dialogs_jupiter.xml
gamedata/configs/text/fra/st_dialogs_pripyat.xml
gamedata/configs/text/fra/st_dialogs_zaton.xml
gamedata/configs/text/fra/st_dialogs.xml
gamedata/configs/text/fra/st_generate_fnames.xml
gamedata/configs/text/fra/st_generate_snames.xml
gamedata/configs/text/fra/st_items_artefacts.xml
gamedata/configs/text/fra/st_items_equipment.xml
gamedata/configs/text/fra/st_items_outfit.xml
gamedata/configs/text/fra/st_items_quest.xml
gamedata/configs/text/fra/st_items_weapons_upgrades.xml
gamedata/configs/text/fra/st_items_weapons.xml
gamedata/configs/text/fra/st_land_names.xml
gamedata/configs/text/fra/st_mp_mapdesc.xml
gamedata/configs/text/fra/st_mp_speechmenu.xml
gamedata/configs/text/fra/st_mp_teamdesc.xml
gamedata/configs/text/fra/st_quests_general.xml
gamedata/configs/text/fra/st_quests_jupiter.xml
gamedata/configs/text/fra/st_quests_pripyat.xml
gamedata/configs/text/fra/st_quests_zaton.xml
gamedata/configs/text/fra/st_subtitles.xml
gamedata/configs/text/fra/ui_st_credits.xml
gamedata/configs/text/fra/ui_st_inventory.xml
gamedata/configs/text/fra/ui_st_ixray.xml
gamedata/configs/text/fra/ui_st_keybinding.xml
gamedata/configs/text/fra/ui_st_loadscreen.xml
gamedata/configs/text/fra/ui_st_mm.xml
gamedata/configs/text/fra/ui_st_mp.xml
gamedata/configs/text/fra/ui_st_other.xml
gamedata/configs/text/fra/ui_st_pda.xml
gamedata/configs/text/fra/ui_st_screen.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_characters_zzzz_pf_console_cleanup_windows1251.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_items_weapons_pf_attachment_compat.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_lz_iwp_flyaga_zzzz_pf_item_use_encoding.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_lz_new_mechanics_zzzz_pf_item_use_encoding.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_pf_door_qa_zzzz_pf_item_use_encoding.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_pf_qa_point_logger_zzzz_pf_item_use_encoding.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_pf_scientist_bunker_zzzz_pf_item_use_encoding.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_pf_stash_qa_zzzz_pf_item_use_encoding.xml
ixr_addons/~pf-french/configs/text/fra/mod_st_pripyat_full_iwp_zzzz_pf_console_cleanup_windows1251.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_ixray_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_ixray_zzzz_pf_console_cleanup_windows1251.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_keybinding_pf_vehicle_radio.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_keybinding_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_loadscreen_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_mm_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_mm_zzzz_pf_console_cleanup_windows1251.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_other_zzzz_pf_console_cleanup_windows1251.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_pda_pf_pa_loners.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_screen_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/~pf-french/configs/text/fra/mod_ui_st_screen_zzzz_pf_console_cleanup_windows1251.xml
ixr_addons/~pf-french/configs/text/fra/mod_zz_pripyat_full_overrides_pf_metal_detector_range.xml
ixr_addons/~pf-french/configs/text/fra/pf_news_debug.xml
ixr_addons/~pf-french/configs/text/fra/st_achievement.xml
ixr_addons/~pf-french/configs/text/fra/st_characters.xml
ixr_addons/~pf-french/configs/text/fra/st_cop_armor_indicators.xml
ixr_addons/~pf-french/configs/text/fra/st_dcp_vehicle_ammo.xml
ixr_addons/~pf-french/configs/text/fra/st_dialog_manager.xml
ixr_addons/~pf-french/configs/text/fra/st_dialogs_jupiter.xml
ixr_addons/~pf-french/configs/text/fra/st_dialogs_pripyat.xml
ixr_addons/~pf-french/configs/text/fra/st_dialogs_zaton.xml
ixr_addons/~pf-french/configs/text/fra/st_dialogs.xml
ixr_addons/~pf-french/configs/text/fra/st_generate_fnames.xml
ixr_addons/~pf-french/configs/text/fra/st_generate_snames.xml
ixr_addons/~pf-french/configs/text/fra/st_items_artefacts.xml
ixr_addons/~pf-french/configs/text/fra/st_items_dead_air_letters.xml
ixr_addons/~pf-french/configs/text/fra/st_items_equipment.xml
ixr_addons/~pf-french/configs/text/fra/st_items_mutant_parts_lz_nm.xml
ixr_addons/~pf-french/configs/text/fra/st_items_outfit.xml
ixr_addons/~pf-french/configs/text/fra/st_items_quest.xml
ixr_addons/~pf-french/configs/text/fra/st_items_weapons_upgrades.xml
ixr_addons/~pf-french/configs/text/fra/st_items_weapons.xml
ixr_addons/~pf-french/configs/text/fra/st_itm_minimod_lz_nm.xml
ixr_addons/~pf-french/configs/text/fra/st_land_names.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_detector_batteries.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_dynamic_surge_cover.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_iwp_flyaga.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_iwp_gasmasks.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_new_mechanics.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_nlc_art_cooking.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_nlc_artifact_ui_labels.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_orders_field_supplies.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_orders_natural_artifacts.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_pda_hacking.xml
ixr_addons/~pf-french/configs/text/fra/st_lz_pripyat_city_notes.xml
ixr_addons/~pf-french/configs/text/fra/st_mp_mapdesc.xml
ixr_addons/~pf-french/configs/text/fra/st_mp_speechmenu.xml
ixr_addons/~pf-french/configs/text/fra/st_mp_teamdesc.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_active_metal_detectors.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_anomalous_upgrades.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_beacon_dialogs.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_beacon_locator.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_beacon_safe_travel.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_binocular.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_buffer.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_camera.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_city_horrors.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_copyright_music.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_developer_achievement.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_developer_mode.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_door_qa.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_dynamic_stashes.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_emission_lamps.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_f1_item_spawner.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_garland.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hawaiian_stash_sales.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_appliances.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_building.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_bulb.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_campfire_cooking.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_campfires.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_cooked_meat.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_encyclopedia.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_even_more_items.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_expansion_items.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_expansion.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_furniture_items.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_furniture.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_interaction.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_map.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_map2.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_map3.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_native_doors.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_online_tv.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_radio_controls.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_radio_playlists.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_raw_meat.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_repair.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_stock.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_hf_workbench.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_interiors.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_inventory_ui_dialogs.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_channel_controls.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_chat.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_multichannel.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_network.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_npcs.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_profiles.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_irc_translate.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_jump_isolator.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_lore_coordinates.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_metal_detector.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_metal_part_repair_balance.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_minimap_settings.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_mod_options.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_monolith_hello.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_nes.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_nocturne_route.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_nocturne.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_novikov_anomaly_dialog.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_novikov_upgrade_access.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_npc_editor.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_ns.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_on_the_edge.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pa_dialogs.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pa_help.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pa_options.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pa_rumors.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_past_memorial.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pda_achievements.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pda_hacking_boot_ui.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pda_player_stats.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pda_vip.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_pda3d.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_photo_cards.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_photographer.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_proc_group_markers.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_puzzles_net.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_puzzles_solo.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_puzzles_unequal.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_puzzles_wordle.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_puzzles.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_qa_point_logger.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_quest_statistics_fixes.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_runtime_editor.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_save_integrity.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_scientist_bunker.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_shift_transfer.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_sleep_timelapse.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_sleepbag_balance.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_smoke.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_st_watch.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_stash_encyclopedia.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_stash_qa.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_stash_settings.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_story_continuation.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_story_documents.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_story_encounter.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_thirst.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_transparent_ui.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_travel.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_trees.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_unique_keys.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_user_gps.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_user_map_filter.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_user_map_markers.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_vehicle_hud.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_vehicle_service.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_vehicle_settings.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_video_settings.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_yanov_freedom_residents.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_yanov_nimble.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_yanov_nitro_services.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_yanov_radio.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_yanov_savka.xml
ixr_addons/~pf-french/configs/text/fra/st_pf_yanov.xml
ixr_addons/~pf-french/configs/text/fra/st_pripyat_full_iwp.xml
ixr_addons/~pf-french/configs/text/fra/st_pripyat_full_stashes.xml
ixr_addons/~pf-french/configs/text/fra/st_quests_general.xml
ixr_addons/~pf-french/configs/text/fra/st_quests_jupiter.xml
ixr_addons/~pf-french/configs/text/fra/st_quests_pripyat.xml
ixr_addons/~pf-french/configs/text/fra/st_quests_zaton.xml
ixr_addons/~pf-french/configs/text/fra/st_subtitles.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_credits.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_inventory.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_ixray.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_keybinding.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_loadscreen.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_mm.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_mp.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_other.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_pda.xml
ixr_addons/~pf-french/configs/text/fra/ui_st_screen.xml
ixr_addons/~pf-french/configs/text/fra/zz_pf_field_archive.xml
ixr_addons/~pf-french/configs/text/fra/zz_pf_loading_thoughts.xml
ixr_addons/~pf-french/configs/text/fra/zz_pripyat_full_overrides.xml
ixr_addons/~pf-french/configs/text/fra/zzz_pf_experimental_far_view.xml
ixr_addons/~pf-french/configs/text/fra/zzz_pf_inventory_box_override.xml
ixr_addons/~pf-french/configs/text/fra/zzz_pf_loading_progress.xml
ixr_addons/~pf-french/configs/text/fra/zzz_pf_stash_statistics.xml
ixr_addons/~pf-french/configs/text/fra/zzzz_pf_vehicle_keybindings.xml
ixr_addons/~pf-french/configs/text/fra/zzzzzz_pf_singleplayer_keybindings.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/fra/zz_pf_field_archive.xml
```

</details>

<details>
<summary>English (eng) · 196 XML paths</summary>

```text
gamedata/configs/text/eng/mod_st_characters_zzzz_pf_console_cleanup_utf8.xml
gamedata/configs/text/eng/mod_ui_st_keybinding_zzzz_pf_console_cleanup_utf8.xml
gamedata/configs/text/eng/pf_news_debug.xml
gamedata/configs/text/eng/st_achievement.xml
gamedata/configs/text/eng/st_characters.xml
gamedata/configs/text/eng/st_dcp_vehicle_ammo.xml
gamedata/configs/text/eng/st_dialog_manager.xml
gamedata/configs/text/eng/st_dialogs_jupiter.xml
gamedata/configs/text/eng/st_dialogs_pripyat.xml
gamedata/configs/text/eng/st_dialogs_zaton.xml
gamedata/configs/text/eng/st_dialogs.xml
gamedata/configs/text/eng/st_generate_fnames.xml
gamedata/configs/text/eng/st_generate_snames.xml
gamedata/configs/text/eng/st_items_artefacts.xml
gamedata/configs/text/eng/st_items_dead_air_letters.xml
gamedata/configs/text/eng/st_items_equipment.xml
gamedata/configs/text/eng/st_items_outfit.xml
gamedata/configs/text/eng/st_items_quest.xml
gamedata/configs/text/eng/st_items_weapons_upgrades.xml
gamedata/configs/text/eng/st_items_weapons.xml
gamedata/configs/text/eng/st_land_names.xml
gamedata/configs/text/eng/st_mp_mapdesc.xml
gamedata/configs/text/eng/st_mp_speechmenu.xml
gamedata/configs/text/eng/st_mp_teamdesc.xml
gamedata/configs/text/eng/st_pf_active_metal_detectors.xml
gamedata/configs/text/eng/st_pf_copyright_music.xml
gamedata/configs/text/eng/st_pf_developer_achievement.xml
gamedata/configs/text/eng/st_pf_developer_mode.xml
gamedata/configs/text/eng/st_pf_door_qa.xml
gamedata/configs/text/eng/st_pf_dynamic_stashes.xml
gamedata/configs/text/eng/st_pf_hawaiian_stash_sales.xml
gamedata/configs/text/eng/st_pf_minimap_settings.xml
gamedata/configs/text/eng/st_pf_mod_options.xml
gamedata/configs/text/eng/st_pf_on_the_edge.xml
gamedata/configs/text/eng/st_pf_pda_achievements.xml
gamedata/configs/text/eng/st_pf_pda_player_stats.xml
gamedata/configs/text/eng/st_pf_pda_vip.xml
gamedata/configs/text/eng/st_pf_save_integrity.xml
gamedata/configs/text/eng/st_pf_scientist_bunker.xml
gamedata/configs/text/eng/st_pf_shift_transfer.xml
gamedata/configs/text/eng/st_pf_stash_qa.xml
gamedata/configs/text/eng/st_pf_stash_settings.xml
gamedata/configs/text/eng/st_pf_story_continuation.xml
gamedata/configs/text/eng/st_pf_story_documents.xml
gamedata/configs/text/eng/st_pf_story_encounter.xml
gamedata/configs/text/eng/st_pf_thirst.xml
gamedata/configs/text/eng/st_pf_user_map_markers.xml
gamedata/configs/text/eng/st_pf_vehicle_hud.xml
gamedata/configs/text/eng/st_pf_vehicle_service.xml
gamedata/configs/text/eng/st_pf_vehicle_settings.xml
gamedata/configs/text/eng/st_pf_video_settings.xml
gamedata/configs/text/eng/st_pf_yanov_freedom_residents.xml
gamedata/configs/text/eng/st_pf_yanov_nimble.xml
gamedata/configs/text/eng/st_pf_yanov.xml
gamedata/configs/text/eng/st_pripyat_full_iwp.xml
gamedata/configs/text/eng/st_pripyat_full_stashes.xml
gamedata/configs/text/eng/st_quests_general.xml
gamedata/configs/text/eng/st_quests_jupiter.xml
gamedata/configs/text/eng/st_quests_pripyat.xml
gamedata/configs/text/eng/st_quests_zaton.xml
gamedata/configs/text/eng/st_subtitles.xml
gamedata/configs/text/eng/ui_st_credits.xml
gamedata/configs/text/eng/ui_st_inventory.xml
gamedata/configs/text/eng/ui_st_ixray.xml
gamedata/configs/text/eng/ui_st_keybinding.xml
gamedata/configs/text/eng/ui_st_loadscreen.xml
gamedata/configs/text/eng/ui_st_mm.xml
gamedata/configs/text/eng/ui_st_mp.xml
gamedata/configs/text/eng/ui_st_other.xml
gamedata/configs/text/eng/ui_st_pda.xml
gamedata/configs/text/eng/ui_st_screen.xml
gamedata/configs/text/eng/zz_pripyat_full_overrides.xml
gamedata/configs/text/eng/zzz_pf_experimental_far_view.xml
gamedata/configs/text/eng/zzz_pf_stash_statistics.xml
gamedata/configs/text/eng/zzzz_pf_vehicle_keybindings.xml
gamedata/configs/text/eng/zzzzzz_pf_singleplayer_keybindings.xml
ixr_addons/~bodycam/configs/text/eng/st_pf_bodycam.xml
ixr_addons/~dynamic-hud/configs/text/eng/st_pf_dynamic_hud.xml
ixr_addons/~HideoutFurniture/configs/text/eng/mod_st_lz_new_mechanics_pf_hf_repair.xml
ixr_addons/~HideoutFurniture/configs/text/eng/mod_ui_st_keybinding_pf_vehicle_radio.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_appliances.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_building.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_bulb.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_campfire_cooking.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_campfires.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_cooked_meat.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_encyclopedia.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_even_more_items.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_expansion_items.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_expansion.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_furniture_items.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_furniture.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_interaction.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_map.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_map2.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_map3.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_native_doors.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_nf.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_online_tv.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_radio_controls.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_radio_playlists.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_raw_meat.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_stock.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_hf_workbench.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_runtime_editor.xml
ixr_addons/~HideoutFurniture/configs/text/eng/st_pf_yanov_savka.xml
ixr_addons/~irc-chat/configs/text/eng/st_pf_irc_channel_controls.xml
ixr_addons/~irc-chat/configs/text/eng/st_pf_irc_chat.xml
ixr_addons/~irc-chat/configs/text/eng/st_pf_irc_network.xml
ixr_addons/~irc-chat/configs/text/eng/st_pf_irc_profiles.xml
ixr_addons/~irc-chat/configs/text/eng/st_pf_irc_translate.xml
ixr_addons/~npc-scene/configs/text/eng/st_pf_npc_editor.xml
ixr_addons/~npc-scene/configs/text/eng/st_pf_ns.xml
ixr_addons/~pda3d/configs/text/eng/st_pf_pda3d.xml
ixr_addons/~pf-buffer/configs/text/eng/st_pf_buffer.xml
ixr_addons/~pf-echo/configs/text/eng/st_pf_city_horrors.xml
ixr_addons/~pf-emission-lamps/configs/text/eng/st_pf_emission_lamps.xml
ixr_addons/~pf-french/configs/text/eng/st_pf_french_fallback.xml
ixr_addons/~pf-garland/configs/text/eng/st_pf_garland.xml
ixr_addons/~pf-interiors/configs/text/eng/st_pf_interiors.xml
ixr_addons/~pf-nocturne-check/configs/text/eng/st_pf_nocturne_route.xml
ixr_addons/~pf-pa-quests/configs/text/eng/st_pf_paq.xml
ixr_addons/~pf-photo-cards/configs/text/eng/st_pf_photo_cards.xml
ixr_addons/~pf-photo-story/configs/text/eng/st_pf_camera.xml
ixr_addons/~pf-photo-story/configs/text/eng/st_pf_past_memorial.xml
ixr_addons/~pf-photo-story/configs/text/eng/st_pf_photographer.xml
ixr_addons/~pf-pseudo-alife/configs/text/eng/st_pf_irc_npcs.xml
ixr_addons/~pf-pseudo-alife/configs/text/eng/st_pf_pa_dialogs.xml
ixr_addons/~pf-pseudo-alife/configs/text/eng/st_pf_pa_help.xml
ixr_addons/~pf-pseudo-alife/configs/text/eng/st_pf_pa_options.xml
ixr_addons/~pf-pseudo-alife/configs/text/eng/st_pf_pa_rumors.xml
ixr_addons/~pf-transparent-ui/configs/text/eng/st_pf_transparent_ui.xml
ixr_addons/~pf-travel/configs/text/eng/st_pf_travel.xml
ixr_addons/~pf-trees/configs/text/eng/st_pf_trees.xml
ixr_addons/~sleep-lapse/configs/text/eng/st_pf_sleep_timelapse.xml
ixr_addons/~smoke/configs/text/eng/st_pf_smoke.xml
ixr_addons/~sss/configs/text/eng/st_pf_sss.xml
ixr_addons/~st-watch/configs/text/eng/st_pf_st_watch.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/eng/st_dialogs_zaton.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/eng/st_items_outfit.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/eng/st_items_weapons.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_items_mutant_parts_lz_nm.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_itm_minimod_lz_nm.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_lz_detector_batteries.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_lz_dynamic_surge_cover.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_lz_iwp_flyaga.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_lz_iwp_gasmasks.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_lz_new_mechanics.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_pf_beacon_locator.xml
ixr_addons/zz_lz-new-mechanics/configs/text/eng/st_pf_beacon_safe_travel.xml
ixr_addons/zzz_lz-pda-hacking/configs/text/eng/st_lz_pda_hacking.xml
ixr_addons/zzz_lz-pda-hacking/configs/text/eng/st_pf_pda_hacking_boot_ui.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_cop_armor_indicators.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_lz_nlc_art_cooking.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_lz_nlc_artifact_ui_labels.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_lz_nlc_natural_artifacts.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_lz_orders_field_supplies.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_lz_orders_natural_artifacts.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_lz_pripyat_city_notes.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_anomalous_upgrades.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_beacon_dialogs.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_binocular.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_metal_part_repair_balance.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_monolith_hello.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_novikov_upgrade_access.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_sleepbag_balance.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_stash_encyclopedia.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/eng/st_pf_yanov_nitro_services.xml
ixr_addons/zzzzzzzzz_pf-qa-point-logger/configs/text/eng/st_pf_qa_point_logger.xml
ixr_addons/zzzzzzzzzz_pf-inventory-ui-la-visual-dev/configs/text/eng/st_pf_inventory_ui_dialogs.xml
ixr_addons/zzzzzzzzzz_pf-lore-coordinate-note/configs/text/eng/st_pf_lore_coordinates.xml
ixr_addons/zzzzzzzzzz_pf-procedural-anomalies-test/configs/text/eng/st_pf_novikov_anomaly_dialog.xml
ixr_addons/zzzzzzzzzz_pf-procedural-anomalies-test/configs/text/eng/st_pf_proc_group_markers.xml
ixr_addons/zzzzzzzzzzz_pf-jump-isolator/configs/text/eng/st_pf_jump_isolator.xml
ixr_addons/zzzzzzzzzzzz_pf-unique-keys/configs/text/eng/st_pf_unique_keys.xml
ixr_addons/zzzzzzzzzzzzzzz_pf-f1-item-spawner/configs/text/eng/st_pf_f1_item_spawner.xml
ixr_addons/zzzzzzzzzzzzzzzz_pf-irc-chat/configs/text/eng/st_pf_irc_chat.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/eng/mod_ui_st_ixray_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/eng/mod_ui_st_mm_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/eng/zz_pf_field_archive.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzz_pf-yanov-radio/configs/text/eng/st_pf_yanov_radio.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzz_pf-weapon-attachment-compat/configs/text/eng/mod_st_items_weapons_pf_attachment_compat.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/eng/st_pf_nes.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/eng/st_pf_puzzles_net.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/eng/st_pf_puzzles_solo.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/eng/st_pf_puzzles_unequal.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/eng/st_pf_puzzles_wordle.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/eng/st_pf_puzzles.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-user-map-filter/configs/text/eng/st_pf_user_map_filter.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzz_pf-user-gps-coordinates/configs/text/eng/st_pf_user_gps.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-quest-statistics-fixes/configs/text/eng/st_pf_quest_statistics_fixes.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomaly-scanner/configs/text/eng/st_pf_anomaly_scanner.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomaly-scanner/configs/text/eng/st_pf_guardian_city_notes.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomalous-boots/configs/text/eng/mod_st_lz_new_mechanics_pf_anomalous_boots.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomalous-boots/configs/text/eng/st_pf_anomalous_boots.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-accessibility-captions/configs/text/eng/st_pf_accessibility_captions.xml
```

</details>

<details>
<summary>Russian (rus) · 188 XML paths</summary>

```text
gamedata/configs/text/rus/mod_st_characters_zzzz_pf_console_cleanup_windows1251.xml
gamedata/configs/text/rus/mod_st_pf_door_qa_zzzz_pf_item_use_encoding.xml
gamedata/configs/text/rus/mod_st_pf_scientist_bunker_zzzz_pf_item_use_encoding.xml
gamedata/configs/text/rus/mod_st_pf_stash_qa_zzzz_pf_item_use_encoding.xml
gamedata/configs/text/rus/mod_st_pripyat_full_iwp_zzzz_pf_console_cleanup_windows1251.xml
gamedata/configs/text/rus/mod_ui_st_ixray_zzzz_pf_console_cleanup_windows1251.xml
gamedata/configs/text/rus/mod_ui_st_keybinding_zzzz_pf_console_cleanup_utf8.xml
gamedata/configs/text/rus/mod_ui_st_loadscreen_zzzz_pf_console_cleanup_utf8.xml
gamedata/configs/text/rus/mod_ui_st_mm_zzzz_pf_console_cleanup_windows1251.xml
gamedata/configs/text/rus/mod_ui_st_other_zzzz_pf_console_cleanup_windows1251.xml
gamedata/configs/text/rus/mod_ui_st_screen_zzzz_pf_console_cleanup_windows1251.xml
gamedata/configs/text/rus/pf_news_debug.xml
gamedata/configs/text/rus/st_dcp_vehicle_ammo.xml
gamedata/configs/text/rus/st_items_dead_air_letters.xml
gamedata/configs/text/rus/st_pf_active_metal_detectors.xml
gamedata/configs/text/rus/st_pf_copyright_music.xml
gamedata/configs/text/rus/st_pf_developer_achievement.xml
gamedata/configs/text/rus/st_pf_developer_mode.xml
gamedata/configs/text/rus/st_pf_door_qa.xml
gamedata/configs/text/rus/st_pf_dynamic_stashes.xml
gamedata/configs/text/rus/st_pf_hawaiian_stash_sales.xml
gamedata/configs/text/rus/st_pf_metal_detector.xml
gamedata/configs/text/rus/st_pf_minimap_settings.xml
gamedata/configs/text/rus/st_pf_mod_options.xml
gamedata/configs/text/rus/st_pf_on_the_edge.xml
gamedata/configs/text/rus/st_pf_pda_achievements.xml
gamedata/configs/text/rus/st_pf_pda_player_stats.xml
gamedata/configs/text/rus/st_pf_pda_vip.xml
gamedata/configs/text/rus/st_pf_save_integrity.xml
gamedata/configs/text/rus/st_pf_scientist_bunker.xml
gamedata/configs/text/rus/st_pf_shift_transfer.xml
gamedata/configs/text/rus/st_pf_stash_qa.xml
gamedata/configs/text/rus/st_pf_stash_settings.xml
gamedata/configs/text/rus/st_pf_story_continuation.xml
gamedata/configs/text/rus/st_pf_story_documents.xml
gamedata/configs/text/rus/st_pf_story_encounter.xml
gamedata/configs/text/rus/st_pf_thirst.xml
gamedata/configs/text/rus/st_pf_user_map_markers.xml
gamedata/configs/text/rus/st_pf_vehicle_hud.xml
gamedata/configs/text/rus/st_pf_vehicle_service.xml
gamedata/configs/text/rus/st_pf_vehicle_settings.xml
gamedata/configs/text/rus/st_pf_video_settings.xml
gamedata/configs/text/rus/st_pf_yanov_freedom_residents.xml
gamedata/configs/text/rus/st_pf_yanov_nimble.xml
gamedata/configs/text/rus/st_pf_yanov.xml
gamedata/configs/text/rus/st_pripyat_full_iwp.xml
gamedata/configs/text/rus/st_pripyat_full_stashes.xml
gamedata/configs/text/rus/ui_st_inventory.xml
gamedata/configs/text/rus/ui_st_ixray.xml
gamedata/configs/text/rus/ui_st_loadscreen.xml
gamedata/configs/text/rus/ui_st_mm.xml
gamedata/configs/text/rus/zz_pf_loading_thoughts.xml
gamedata/configs/text/rus/zz_pripyat_full_overrides.xml
gamedata/configs/text/rus/zzz_pf_experimental_far_view.xml
gamedata/configs/text/rus/zzz_pf_inventory_box_override.xml
gamedata/configs/text/rus/zzz_pf_loading_progress.xml
gamedata/configs/text/rus/zzz_pf_stash_statistics.xml
gamedata/configs/text/rus/zzzz_pf_vehicle_keybindings.xml
gamedata/configs/text/rus/zzzzzz_pf_singleplayer_keybindings.xml
ixr_addons/~bodycam/configs/text/rus/st_pf_bodycam.xml
ixr_addons/~dynamic-hud/configs/text/rus/st_pf_dynamic_hud.xml
ixr_addons/~HideoutFurniture/configs/text/rus/mod_st_lz_new_mechanics_pf_hf_repair.xml
ixr_addons/~HideoutFurniture/configs/text/rus/mod_ui_st_keybinding_pf_vehicle_radio.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_appliances.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_building.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_bulb.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_campfire_cooking.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_campfires.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_cooked_meat.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_encyclopedia.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_even_more_items.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_expansion_items.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_expansion.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_furniture_items.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_furniture.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_interaction.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_map.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_map2.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_map3.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_native_doors.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_nf.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_online_tv.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_radio_controls.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_radio_playlists.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_raw_meat.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_stock.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_hf_workbench.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_runtime_editor.xml
ixr_addons/~HideoutFurniture/configs/text/rus/st_pf_yanov_savka.xml
ixr_addons/~irc-chat/configs/text/rus/st_pf_irc_channel_controls.xml
ixr_addons/~irc-chat/configs/text/rus/st_pf_irc_chat.xml
ixr_addons/~irc-chat/configs/text/rus/st_pf_irc_multichannel.xml
ixr_addons/~irc-chat/configs/text/rus/st_pf_irc_network.xml
ixr_addons/~irc-chat/configs/text/rus/st_pf_irc_profiles.xml
ixr_addons/~irc-chat/configs/text/rus/st_pf_irc_translate.xml
ixr_addons/~npc-scene/configs/text/rus/mod_st_pf_npc_editor_zzzz_pf_item_use_encoding.xml
ixr_addons/~npc-scene/configs/text/rus/st_pf_npc_editor.xml
ixr_addons/~npc-scene/configs/text/rus/st_pf_ns.xml
ixr_addons/~pda3d/configs/text/rus/st_pf_pda3d.xml
ixr_addons/~pf-buffer/configs/text/rus/st_pf_buffer.xml
ixr_addons/~pf-echo/configs/text/rus/st_pf_city_horrors.xml
ixr_addons/~pf-emission-lamps/configs/text/rus/st_pf_emission_lamps.xml
ixr_addons/~pf-garland/configs/text/rus/st_pf_garland.xml
ixr_addons/~pf-interiors/configs/text/rus/st_pf_interiors.xml
ixr_addons/~pf-nocturne-check/configs/text/rus/st_pf_nocturne_route.xml
ixr_addons/~pf-nocturne/configs/text/rus/st_pf_nocturne.xml
ixr_addons/~pf-pa-quests/configs/text/rus/st_pf_paq.xml
ixr_addons/~pf-photo-cards/configs/text/rus/st_pf_photo_cards.xml
ixr_addons/~pf-photo-story/configs/text/rus/st_pf_camera.xml
ixr_addons/~pf-photo-story/configs/text/rus/st_pf_past_memorial.xml
ixr_addons/~pf-photo-story/configs/text/rus/st_pf_photographer.xml
ixr_addons/~pf-pseudo-alife/configs/text/rus/mod_ui_st_pda_pf_pa_loners.xml
ixr_addons/~pf-pseudo-alife/configs/text/rus/st_pf_irc_npcs.xml
ixr_addons/~pf-pseudo-alife/configs/text/rus/st_pf_pa_dialogs.xml
ixr_addons/~pf-pseudo-alife/configs/text/rus/st_pf_pa_help.xml
ixr_addons/~pf-pseudo-alife/configs/text/rus/st_pf_pa_options.xml
ixr_addons/~pf-pseudo-alife/configs/text/rus/st_pf_pa_rumors.xml
ixr_addons/~pf-transparent-ui/configs/text/rus/st_pf_transparent_ui.xml
ixr_addons/~pf-travel/configs/text/rus/st_pf_travel.xml
ixr_addons/~pf-trees/configs/text/rus/st_pf_trees.xml
ixr_addons/~sleep-lapse/configs/text/rus/st_pf_sleep_timelapse.xml
ixr_addons/~smoke/configs/text/rus/st_pf_smoke.xml
ixr_addons/~sss/configs/text/rus/st_pf_sss.xml
ixr_addons/~st-watch/configs/text/rus/st_pf_st_watch.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/rus/mod_st_dialogs_zaton_stcopwp.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/rus/st_items_outfit.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/rus/st_items_weapons_stcopwp.xml
ixr_addons/ixray-stcop-wp-3.7-based-r1.0/configs/text/rus/st_items_weapons.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/mod_st_lz_iwp_flyaga_zzzz_pf_item_use_encoding.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/mod_st_lz_new_mechanics_zzzz_pf_item_use_encoding.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_items_mutant_parts_lz_nm.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_itm_minimod_lz_nm.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_lz_detector_batteries.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_lz_dynamic_surge_cover.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_lz_iwp_flyaga.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_lz_iwp_gasmasks.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_lz_new_mechanics.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_pf_beacon_locator.xml
ixr_addons/zz_lz-new-mechanics/configs/text/rus/st_pf_beacon_safe_travel.xml
ixr_addons/zzz_lz-pda-hacking/configs/text/rus/st_lz_pda_hacking.xml
ixr_addons/zzz_lz-pda-hacking/configs/text/rus/st_pf_pda_hacking_boot_ui.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/mod_zz_pripyat_full_overrides_pf_metal_detector_range.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_cop_armor_indicators.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_lz_nlc_art_cooking.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_lz_nlc_artifact_ui_labels.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_lz_nlc_natural_artifacts.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_lz_orders_field_supplies.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_lz_orders_natural_artifacts.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_lz_pripyat_city_notes.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_anomalous_upgrades.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_beacon_dialogs.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_binocular.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_metal_part_repair_balance.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_monolith_hello.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_novikov_upgrade_access.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_sleepbag_balance.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_stash_encyclopedia.xml
ixr_addons/zzzzzzzzz_lz-nlc-art-cooking-dev/configs/text/rus/st_pf_yanov_nitro_services.xml
ixr_addons/zzzzzzzzz_pf-qa-point-logger/configs/text/rus/mod_st_pf_qa_point_logger_zzzz_pf_item_use_encoding.xml
ixr_addons/zzzzzzzzz_pf-qa-point-logger/configs/text/rus/st_pf_qa_point_logger.xml
ixr_addons/zzzzzzzzzz_pf-inventory-ui-la-visual-dev/configs/text/rus/st_pf_inventory_ui_dialogs.xml
ixr_addons/zzzzzzzzzz_pf-lore-coordinate-note/configs/text/rus/st_pf_lore_coordinates.xml
ixr_addons/zzzzzzzzzz_pf-procedural-anomalies-test/configs/text/rus/st_pf_novikov_anomaly_dialog.xml
ixr_addons/zzzzzzzzzz_pf-procedural-anomalies-test/configs/text/rus/st_pf_proc_group_markers.xml
ixr_addons/zzzzzzzzzzz_pf-jump-isolator/configs/text/rus/st_pf_jump_isolator.xml
ixr_addons/zzzzzzzzzzzz_pf-unique-keys/configs/text/rus/st_pf_unique_keys.xml
ixr_addons/zzzzzzzzzzzzzzz_pf-f1-item-spawner/configs/text/rus/st_pf_f1_item_spawner.xml
ixr_addons/zzzzzzzzzzzzzzzz_pf-irc-chat/configs/text/rus/st_pf_irc_chat.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/rus/mod_ui_st_ixray_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/rus/mod_ui_st_mm_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/rus/mod_ui_st_screen_zzzz_pf_console_cleanup_utf8.xml
ixr_addons/zzzzzzzzzzzzzzzzzz_pf-field-archive-menu/configs/text/rus/zz_pf_field_archive.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzz_pf-yanov-radio/configs/text/rus/st_pf_yanov_radio.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzz_pf-weapon-attachment-compat/configs/text/rus/mod_st_items_weapons_pf_attachment_compat.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/rus/st_pf_nes.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/rus/st_pf_puzzles_net.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/rus/st_pf_puzzles_solo.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/rus/st_pf_puzzles_unequal.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/rus/st_pf_puzzles_wordle.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-puzzles/configs/text/rus/st_pf_puzzles.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzz_pf-user-map-filter/configs/text/rus/st_pf_user_map_filter.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzz_pf-user-gps-coordinates/configs/text/rus/st_pf_user_gps.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-quest-statistics-fixes/configs/text/rus/st_pf_quest_statistics_fixes.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomaly-scanner/configs/text/rus/st_pf_anomaly_scanner.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomaly-scanner/configs/text/rus/st_pf_guardian_city_notes.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomalous-boots/configs/text/rus/mod_st_lz_new_mechanics_pf_anomalous_boots.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-anomalous-boots/configs/text/rus/st_pf_anomalous_boots.xml
ixr_addons/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_pf-accessibility-captions/configs/text/rus/st_pf_accessibility_captions.xml
```

</details>

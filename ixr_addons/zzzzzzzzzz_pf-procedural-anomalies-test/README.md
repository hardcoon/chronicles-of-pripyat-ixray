# Fixed Static Anomalies

Production fixed population for `pripyat_full`, active in normal games and in
developer mode. Game difficulty does not enable or disable it.

- The only layout is the approved fixed seed `20260821`; emissions do not
  randomize, retire, move, or recreate it.
- The layout has 1,212 point anomalies: 612 in 41 named groups and 600
  solitary anomalies. The 41 main and 15 mixed group shells remain baked as
  marker geometry. Radiation, chemical, psi and thermal shells also provide
  background danger; gravitational and electric shells stay neutral.
- Every shell keeps idle particles, light and sound disabled. Chemical and
  thermal shells deliberately have no screen postprocess; psi and radiation
  retain their native warning effect. All four damaging shell types are
  registered with the HUD danger indicators; only radiation retains geiger
  clicks, while the other three detector sounds are silenced by the late
  detector override.
- The legacy mode name `static_spawn_test` selects the verified baked spawn.
  Existing saves that already contain the population are supported.
- All 1,268 server objects are baked into `all.spawn` and `level.spawn`.
  Lua never creates, releases, relocates, or regenerates them.
- Native ALife keeps distant zones offline (100/125 m hysteresis) and permits
  no more than one new online transition per 50 ms.
- Native transitions pause after frames over 50 ms and skip frames over 40 ms.
  An exhaustive density check proves that any 100 m actor-centred disk can
  contain at most 47 procedural shells, including at most 3 auras, so mutable
  online counters and their save/load synchronization are unnecessary.
- `se_zones` directly owns that policy. There is no actor-update object
  spawner, saved runtime-ID registry, layout transaction or new-game callback.
- Zone resources are warmed one profile per 100 ms through the existing
  anomaly-effect update path before native online transitions are permitted.
- All 41 groups retain their main shell; the 15 approved mixed groups retain a
  secondary shell. Only radiation, chemical, psi and thermal shells damage the
  actor; none of the group shells uses `strike` or `shock` damage.
- The 41 main aura objects anchor serialized vanilla `primary_object` PDA
  markers, but every coordinate starts hidden. A marker is permanently
  discovered only after the actor physically enters its main aura; discovery
  is saved in one compact actor `pstor` value. Novikov's coordinate service
  calls `pf_proc_group_markers.reveal_group(group_id)` after a successful
  1,000-ruble purchase; already discovered groups are omitted from its lists.
- Artifact spawning and cooking are intentionally absent.
- Runtime coordinates are validated against the active `level.ai`, cross table,
  `level.cform`, stacked-navigation/interior masks, roads, and manual exclusions.
- Do not reuse saves after changing mode or restoring a different spawn.
- The addon does not override `scripts/ixray_system/dynamic_callbacks.lua`.

The source compiler, manifests, spawn candidates, backup, verification reports,
and rollback script live under
`.ixray-local/workspace/tools/anomaly_generation_preview`.

The static population requires a full engine restart after script or config
changes. A new game is needed only after replacing `all.spawn` or
`level.spawn` with a different population.

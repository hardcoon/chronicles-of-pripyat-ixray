<p align="center">
  <img src="https://ap-pro.ru/uploads/monthly_2026_08/641841139-227bfa1d-fe52-4465-9092-7513cd6053bd.png.c22609badf148d9c95f242fc6645ba41.png" alt="S.T.A.L.K.E.R. — Chronicles of Pripyat" width="1024">
</p>

# Chronicles of Pripyat — IX-Ray

A standalone S.T.A.L.K.E.R. project about exploring Greater Pripyat, finding what the city has left behind, and making your own way through the Zone.

| Project information | |
| --- | --- |
| Engine | Modified IX-Ray |
| Installation | Standalone |
| Original release | August 27, 2026 |
| Current game build | `2026.11.25-dev.123` |
| Launcher | `1.6.0` |
| Optional Winter addon | `1.65` — installed separately through the launcher |

[Download the launcher](https://github.com/hardcoon/chronicles-of-pripyat-ixray/releases/download/dev-2026.08.26.1/ChroniclesLauncher.exe) · [Official AP-PRO thread](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/) · [Screenshots](https://disk.yandex.ru/d/tP9oWk9lmLc-Wg)

<a id="about-the-project"></a>
<details>
<summary><strong>About the project</strong></summary>

**Chronicles of Pripyat** is a gameplay and narrative project built on IX-Ray. You play as the Unknown, who comes to Pripyat in search of the past he lost there.

There is no mandatory linear storyline. Free exploration and survival are the foundation of the game. Notes tell the stories of Pripyat's residents and of what was hidden behind the Iron Curtain. Much of this material comes from documented sources, adapted to an in-game format; the main quest's story is fiction.

My aim is to show a more real, often forgotten side of the Zone: a city worth exploring, rather than a backdrop. Building on the original Greater Pripyat map, I wanted to introduce new mechanics and bring more players and modders to it.

The project is primarily for people interested in the original map author's concept: **Living Zone OBT 4.26**. You can find the [original project's thread](https://ap-pro.ru/forums/topic/4023-zhivaya-zona-obt/) and the [map author's VK community](https://vk.com/projektx) here.

If you are looking for an elaborate story campaign, constant action, or an in-depth survival simulator, this is not that kind of mod.

</details>

<a id="main-features"></a>
<details>
<summary><strong>Main features</strong></summary>

- Free exploration of Pripyat without a compulsory linear campaign.
- Notes about the city's residents and their lives after the disaster.
- Hand-placed inhabitants and more than 1,000 logical stash spawn locations.
- Tiered stashes, with unique gameplay-changing items in the higher tiers.
- Adapted vehicles, including interaction physics, sounds, and fuel systems.
- PDA hacking through a minigame.
- Reworked anomaly locations and 41 anomaly groups with artifact spawns and discoverable map markers. An [anomaly map](https://disk.yandex.ru/i/jtvSX_Dhx6Dofw) is available separately.
- Artifact transmutation adapted from NLC, with properties balanced for Call of Pripyat.
- A redesigned interface using work from Lost Alpha Enhanced Edition.
- Populated Yanov and the Bunker, with several important characters.
- Renderer changes aimed at improving performance.
- Route-based travel on foot or by vehicle through the **Direction Finder** PDA tab.
- A way to sell surplus loot through the **Orders** tab.
- An adapted Hideout Furniture system, with workbenches, campfires, and cooking.
- Six PDA minigames: five puzzle games and Tank 1990.
- Anomalous activity scanners for finding artifacts remotely.
- In-game chat with other players.
- Many other changes that affect how the game plays.

For a look at the game, browse the [screenshot gallery](https://disk.yandex.ru/d/tP9oWk9lmLc-Wg). Player-made interiors have their own [gallery and discussion thread](https://ap-pro.ru/forums/topic/15559-chronicles-of-pripyat-gotovye-interery/).

</details>

<a id="system-requirements"></a>
<details>
<summary><strong>System requirements</strong></summary>

The following is the project's baseline configuration for **1920 × 1080 at low settings**. Performance varies with the scene and settings; no fixed frame rate is guaranteed.

| Component | Minimum / recommended setup from the project guide |
| --- | --- |
| OS | Windows 10 or Windows 11, 64-bit |
| CPU | Intel Core i5-4460 / AMD Ryzen 3 1200 |
| RAM | 16 GB |
| GPU | NVIDIA GeForce GTX 1060 6 GB / AMD Radeon RX 580 8 GB |
| Space for the base game | Allow at least 25 GB, plus room for downloads, temporary files, saves, and caches |
| Optional Winter addon | Approximately 8.35 GB of installed files, plus download and temporary space |
| System-drive headroom | The project guide recommends keeping approximately 64 GB free for Windows, paging, and temporary files |
| Page file | Enabled and system-managed |

Required software:

- [OpenAL Driver](https://www.openal.org/downloads/)
- [Current Microsoft Visual C++ v14 Redistributable, x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist/)
- [DirectX End-User Runtime](https://www.microsoft.com/en-us/download/details.aspx?id=35)

For memory-related crashes, see [Troubleshooting](#troubleshooting).

</details>

<a id="installation-and-updates"></a>
<details>
<summary><strong>Installation and updates</strong></summary>

1. Download [ChroniclesLauncher.exe](https://github.com/hardcoon/chronicles-of-pripyat-ixray/releases/download/dev-2026.08.26.1/ChroniclesLauncher.exe).
2. Run it from a folder you can write to.
3. Use **Game folder** to choose the installation location, then press **INSTALL GAME**.

The launcher creates a separate `Chronicles of Pripyat 4.26 IX-Ray` folder. After installation, it copies itself into that folder and creates a **Chronicles of Pripyat** desktop shortcut.

It installs the complete base game; you do not need to apply earlier patches first. **Code → Download ZIP** on GitHub is not a complete game download: this repository tracks the project's files, while the full installation is delivered through release packages.

**PLAY** starts your installed game and works without an internet connection. It does not require a file scan before every launch.

### Updates and file verification are separate

- **Check for updates** checks whether a new game build is available. It does not perform a full hash scan just because you are already up to date.
- The optional startup update notification can be configured in **About**. It checks update metadata, not the contents of your game files.
- **Verify files**, in the left sidebar, checks the installed game's hashes and managed addons. It reports mismatches; it does not automatically install an update or repair files.

The launcher can download from **GitHub** or **Gitea**. Select the source from **Download source** in the left sidebar. Availability depends on your connection and provider. If you cannot reach the servers, use the fallback download below; receiving subsequent patches still requires access to a working download source. I cannot upload every new build to Yandex Disk immediately.

**Fallback:** [prebuilt game on Yandex Disk — `2026.10.31-dev.106`](https://disk.yandex.ru/d/Pyh2GZiydupJKw). This is an older, ready-to-play build, not the current release.

### Winter is a separate addon

The Winter addon is **not included in the base game or ordinary game patches**. It is available separately in the launcher's **Addons** section; use launcher **1.5.2 or newer** for the current update behavior.

Selecting the addon installs and enables it. Clearing its checkbox keeps the files and disables it by renaming `addon.init` to `addon.init_off`. Enabled, launcher-managed addons receive their own updates.

With saved version metadata, launcher 1.5.2 compares manifests and downloads changed files without rehashing or copying the entire addon. It does not create a full addon backup. Older installations without reference metadata may need a one-time scan; manual verification remains separate.

</details>

<a id="graphics-performance-and-the-map"></a>
<details>
<summary><strong>Graphics, performance, and the map</strong></summary>

### A much larger Pripyat

| Location | Approximate dimensions | Approximate area |
| --- | --- | --- |
| Original Call of Pripyat: Pripyat | 1,100 × 1,100 m | 1.21 km² |
| Jupiter | 1,200 × 1,200 m | 1.44 km² |
| Greater Pripyat | 3,818 × 3,932 m | 15.01 km² |

Greater Pripyat is roughly **10 times the area of Jupiter** and **12 times the area of the original Pripyat**. The scale matters when comparing performance with the original game.

### R4 and view-distance presets

Only the **R4 renderer** is supported. It has been modified to improve performance; there is no R1/static-lighting mode.

The **View distance (R4)** setting offers several approaches:

1. **Use slider:** adjust **Full-detail distance (R4)** manually.
2. **Original — performance:** a performance-oriented view-distance preset.
3. **Extended — performance:** a longer-distance alternative.

If performance is poor, start with **Original — performance** and adjust from there. Higher-distance presets are also available for comparison and screenshots.

Development references: [OpenXRay / modified IX-Ray R4 comparison](https://www.diffchecker.com/image-compare/xYq7GMzm/) and [R4 optimization notes](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563494).

### Visuals

The visual foundation is IX-Ray rather than a wholesale high-resolution graphics overhaul. [Paradox Weather](https://github.com/ixray-community/ixray-paradox-weather-unit) supplies the sky textures and weather cycles. A couple of cycles were shifted toward blue because the original green tint was too strong.

The first-install antialiasing preset is **TAA + SMAA for the hands**. It combines a temporally smoothed scene with separate treatment of the hands to reduce visible TAA trails.

Selected SSS effects have also been adapted: puddles, tree shaders, and grass shaders. The aim is a visible improvement without requiring an especially powerful PC. These options can be disabled.

<details>
<summary>Performance work</summary>

The project targets one renderer instead of maintaining several incompatible graphics paths. Optimization work includes:

- Fixing vanilla NPC pathfinding problems that caused noticeable stutters when NPCs entered combat.
- Replacing dynamic lamps and base lighting with static lighting.
- Disabling base lights completely when the player is far away.
- Preparing performance-oriented view-distance presets.
- Streaming stash containers from logical records instead of creating every possible stash at once.
- Spreading anomaly, particle, and sound activation over time to reduce sudden load.
- Fixing a stash-spawn vertex search that previously scanned the entire map.

</details>

</details>

<a id="gameplay-guide"></a>
<details>
<summary><strong>Gameplay guide</strong></summary>

The in-game **Encyclopedia** is the main reference for the game's mechanics. The essentials are summarized below.

### Starting out

At the start of a new game, go straight ahead to the guide and the checkpoint. This starts the main quest and opens checkpoint-related systems, including Orders. The game expects you to find your way into the city rather than follow a marker for every step.

<details>
<summary>Vehicles: fuel, condition, and the complete purchase list</summary>

Look for **Nimble at Yanov** if you want to buy a vehicle. A car is useful, but it is not a solution to everything.

- Fuel consumption has been reduced. Nimble offers fuel cans roughly once every **3–6 days**.
- Vehicles automatically regenerate health up to **20%**.
- There are no vehicle repair kits. If you manage to destroy your car, discard it and buy another one.
- A gear icon at the bottom right shows vehicle condition: gray above **50%**, red below **10%**.
- Handling and performance do not deteriorate with health. A vehicle at **5%** drives the same way as one at **100%**.
- A starter vehicle is placed on the map if you prefer driving to teleporting. Look for it.
- The **Spawn vehicles in the game** option places the available vehicles on the map as if you had bought them from Nimble. It is there for players who do not want to save up, use the item spawner, or who simply find that arrangement more natural.

Vehicles available for purchase:

| Category | Models |
| --- | --- |
| Light vehicles | ZAZ-968M; VAZ-2101; Izh / Moskvich-412; Moskvich-2715; RAF-2203; VAZ-2107; GAZ-24 Volga; VAZ-2108; Mercedes W123; ZAZ-965; Moskvich-408 |
| Off-road vehicles | LuAZ; GAZ-69; rusty white Niva-2121; green Niva-2121; rusty Bukhanka; UAZ Hunter; UAZ-452 Bukhanka; Niva-2329 |
| Heavy vehicles | ZIL-130; PAZ bus; ZIL-131; military KamAZ with a canvas cover; KamAZ without a canvas cover |
| Armored vehicles | Turretless BRDM; BRDM-2; BTR |

BRDM and BTR purchases unlock after completing **20 levels of Tank 1990**.

The spawner also contains vehicles marked **legacy**, so you can compare them with the original versions. Other vehicles were left out for a variety of reasons, from the difficulty of porting them to whether they made sense on this map.

</details>

<details>
<summary>Notes, the story, and the Encyclopedia</summary>

- Story notes have a **20% chance** to appear in **blue-tier stashes and above**.
- Once a note has been assigned to a stash, it stays associated with that stash.
- Read carefully. Following the notes leads to an achievement, an open ending, and a small additional reward. If you only want to wander around, it is possible to miss the less obvious solution.
- Do not rush to throw away the non-main-story notes: there are two collections of **36 and 44**. They become useful after the achievement and the main storyline.
- The **22 main-story notes** can be handed to a particular character at the end of the story.
- The historical and personal accounts in the other notes were adapted from real sources. The main quest's story notes are the exception: that narrative is fictional.
- Every note is also recorded in the in-game Encyclopedia.
- The **Sheet with Coordinates** appears in **purple-tier stashes and above**. Using it marks a stash containing a note. It can help you find that last missing note or simply speed up the search.

</details>

<details>
<summary>Finding stashes, detector guarantees, and the container limit</summary>

There are many ways to find stashes:

1. Explore buildings yourself.
2. Use the **Homemade**, **Wave**, and **Magnet** metal detectors — not to be confused with the detector you receive at the start.
3. Buy leads from **Hawaiian**.
4. Hack PDAs.
5. Reflash locked PDAs.
6. Complete minigames.
7. Use **Strange** items; read their Encyclopedia entries.
8. Reach the appropriate ending to unlock an additional way to acquire red stashes.
9. Search existing stashes: they can contain leads to other stashes.

Your first metal detector is guaranteed in your **first purple stash**. Bad-luck protection also applies: after **15 unsuccessful searches** of purple, legendary/orange, or red stashes, the next qualifying search guarantees a higher-tier metal detector. The USB drive used for hacking is guaranteed in your **second purple stash**.

For performance, the game has a hard limit of **256 physical stash containers at once**. A new game selects **400 stash locations** from the prepared logical points. Containers are created gradually when you approach them or receive a lead, as long as there is room within the physical-container limit. The main contents are determined when the container is first created.

A discovered stash retains its contents and occupies one of those 256 slots. **Empty stashes completely** to make room for new ones. If you accumulate 256 discovered, non-empty stashes, new containers stop appearing until you free a slot. Existing stashes do not burn up or disappear; you simply stop getting new leads through ordinary exploration when the limit is reached.

Once a stash is completely empty and you leave the area, the system chooses a new location for it.

</details>

<details>
<summary>PDA minigames and their rewards</summary>

There are **five puzzle minigames**, each with three difficulty levels. **Tank 1990** is an additional game and does not follow the same completion-and-reward system.

Completing puzzles earns rewards. The reward is determined when the minigame starts, so reloading does not reroll it.

| Minigame difficulty | Stash reward chances |
| --- | --- |
| Easy | No stash reward |
| Normal | Green 84%; blue 12%; purple 3%; orange 1% |
| Hard | Green 67%; blue 24%; purple 6%; orange 2%; red 1% |

This is the difficulty selected **inside the minigame**, not the overall Stalker/Veteran game difficulty.

</details>

<details>
<summary>Orders: selling surplus equipment</summary>

The **Orders** tab is a straightforward way to turn spare equipment into money without changing the exploration-focused gameplay.

Deliveries go into prepared containers. There are **25 delivery containers** around the edges of the location; accepting an order assigns an available one.

The board has **five order slots**. Each receives a random refresh hour once, and keeps that hour afterward. Every in-game day, each slot receives a new order at its assigned time. An unaccepted order is replaced. If you accept an order, its board slot remains empty until the next scheduled refresh.

If the game says the checkpoint is not open and will not let you take orders, visit the checkpoint near the new-game starting area first.

</details>

<details>
<summary>PDA hacking: reading the interface</summary>

1. **Actions:** most actions cost one point; attempts to obtain extra stashes cost more.
2. **Scripts:** each script contains a set of hacks. Select a script to inspect its available hacks. Running out of scripts means failure. A number above the hacks also shows how many scripts remain.
3. **Hacks:** selecting one runs it immediately and applies its effect. This consumes the current script, so you can use only one hack from each script.
4. **Protection bar:** affects how much progress hacks add and increases the chances of receiving extra stashes.
5. **Hacking progress:** fill this bar to **100%** to complete the minigame.

The action log is in the center, and the **[?]** button opens help.

![PDA hacking interface, with the numbered areas explained above](https://ap-pro.ru/uploads/monthly_2026_09/hack.jpg.e64f71c5dfc034764c5191fb6c88c18a.jpg)

</details>

<details>
<summary>Equipment repairs and weapon upgrades</summary>

Equipment progression works differently here:

- Technicians no longer perform the usual weapon upgrades. The separate module system replaces that process to avoid conflicts and errors.
- **Novikov** can upgrade armor after you complete **Hermann's quest**. Armor can also be upgraded at a workbench.
- Equipment is repaired using repair items, through technicians, or at a workbench.
- **Nitro's repairs are initially cheaper** than Novikov's after you unlock him.
- Weapon upgrade modules are kits that you drag onto a weapon.
- A weapon can have at most **three upgrades**.
- Unique modules include effects such as **+35% reload speed** or **+10% damage**.
- Modules can be obtained through exchanges with Nitro or found in stashes.

</details>

<details>
<summary>Artifacts, anomaly groups, and transmutation</summary>

The NLC artifact transmutation system has been adapted for this project: throw an artifact into the appropriate anomaly to obtain a different artifact.

- **10 artifacts** spawn at the start of a new game.
- After each emission, up to **three artifacts** from classes **I–IV** are added, without exceeding the world limit.
- The limit is **18 artifacts** initially, **22 from day 5**, and **24 from day 10 onward**.
- The progression includes classes **I, II, III, IV**, followed by **modified artifacts, mesomodified artifacts, hypermodified artifacts, and absolutes**.
- **Class IV** artifacts are the starting point for transmutation.
- The **Echo** detector can find all four initial classes.
- You receive a guaranteed **Bear** detector after the **tenth story note**.
- Detector bad-luck protection guarantees higher-tier drops by the **6th, 11th, and 21st qualifying stash**. A **Svarog** is guaranteed by the **seventh qualifying orange/red stash**.
- Artifacts spawn within the prepared anomaly groups. Discover their map markers yourself or buy the information from **Novikov**.
- **Anomalous activity scanners** help you search remotely. Their stash drop chances are **10% in blue, 25% in purple, 80% in orange, and 100% in red**.
- Buy transmutation recipes from Hermann, or experiment without a recipe.

<details>
<summary>Spoilers: unique finds, the Keeper, and late-game rewards</summary>

- One red stash contains an **Anomaly Analyzer**, which places 3D markers on nearby anomalies.
- One red stash contains **Vortex**, a unique artifact that attracts other artifacts.
- After the tenth story note, an unobtrusive but important character appears. If he dies, you lose access to the associated achievement and opportunities below.
- Hand notes to the **Keeper** to receive anomalous weapon upgrades that let you obtain artifacts.
- Turning in notes also rewards you with a unique suit and artifact.
- The Keeper exchanges artifacts for red stashes, without a fixed limit on the number of exchanges.
- Handing in the story notes after completing the storyline awards another unique artifact.

</details>

</details>

</details>

<a id="in-game-chat"></a>
<details>
<summary><strong>In-game chat</strong></summary>

The **Pripyat — 4** PDA tab is an IRC chat with other players, **not multiplayer or co-op**. Enter a nickname using Latin characters, such as `Nick_123`, and sign in. The default server settings are already supplied.

`pf_rc_bridge.exe` is the chat's network bridge. Normally it starts when you sign in; with automatic login enabled, it starts when you first open a PDA tab. It closes when you exit the game. The chat tab can be disabled in the game settings.

<details>
<summary>Connection troubleshooting</summary>

The configured IRC connection uses **TLS on port 6697**, with `nexus.hybridirc.com` and `irc.hybridirc.com` as server options.

Open PowerShell and test connectivity:

```powershell
Test-NetConnection nexus.hybridirc.com -Port 6697 -InformationLevel Detailed
Test-NetConnection irc.hybridirc.com -Port 6697 -InformationLevel Detailed
```

`TcpTestSucceeded: True` confirms that a TCP connection was possible; it does not by itself prove that IRC login or TLS negotiation succeeded.

If the test fails, compare your connection with and without VPN, zapret, or AdGuard, if you use them. A mobile connection can help distinguish a home-network or provider issue. Check that your firewall and network software allow the game and `pf_rc_bridge.exe` to make outbound connections to the configured server on port 6697. Do not disable your firewall or open inbound router ports just for this test.

When reporting a problem, include:

- Your game build, country, and any VPN or network-filtering software in use.
- The exact error message or a screenshot.
- The complete PowerShell output, not just `True` or `False`.
- `userdata/logs/pf_irc_chat_bridge.log` from the game folder.

</details>

<details>
<summary>Creating a private IRC channel</summary>

For a stream or a separate group:

1. Change the channel name.
2. Set a room key, or leave it blank.
3. Disable **Do not join an unregistered channel**.
4. Clear the **Required modes** field.

A temporary channel disappears when its last user leaves.

![Original IRC settings illustration](https://ap-pro.ru/uploads/monthly_2026_09/irc.jpg.ef456c58d787eb751faf559c2abe79d7.jpg)

</details>

</details>

<a id="developer-tools-hud-and-accessibility"></a>
<details>
<summary><strong>Developer tools, HUD, and accessibility</strong></summary>

<details>
<summary>Item spawner and developer interface</summary>

The built-in spawner is disabled by default. To enable it, open **Settings → Game → Developer mode**. The spawner then becomes available from the game menu with **F1**. It is intended for testing mechanics or recording footage.

The IX-Ray developer GUI is also available with **Ctrl + Shift + F10**.

</details>

<details>
<summary>HUD visibility bindings</summary>

Enter these commands in the game console:

```text
bind_console hud_draw on kF6
bind_console hud_draw off kF7
cfg_save
```

**F6** shows the HUD, **F7** hides it, and `cfg_save` saves the bindings to `user.ltx`. A [single-key alternative](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=578546) is described in the forum.

</details>

<details>
<summary>Sound captions</summary>

Basic sound captions cover NPCs, mutants, anomalies, artifact detectors, and metal detectors. Enable them under **Sound → Sound captions**.

![Sound-caption illustration from the project thread](https://ap-pro.ru/uploads/monthly_2026_09/dZfgIhfDVF.jpg.7a6a1c76b56d2b0fd6a5e313fbdcd9a4.jpg)

</details>

</details>

<a id="interactive-streams"></a>
<details>
<summary><strong>Interactive streams</strong></summary>

[**Chronicles Donation Bridge**](https://github.com/hardcoon/chronicles-donation-bridge) is a separate beta application for DonationAlerts-driven interactive streams. It was made for this project; compatibility with unrelated mods is not implied. An Anomaly version is also available in its repository.

The application receives DonationAlerts events and invokes in-game scripts. Its test controls let you check those script actions independently. It is provided as-is, without a promise of further development.

<details>
<summary>DonationAlerts API correspondence</summary>

In the correspondence shown in the project thread, DonationAlerts support confirmed that this use of the API did not require additional registration and was not expected to cause account blocks or frozen payouts. This records their reply, rather than making a guarantee about future service policies.

![DonationAlerts support's reply, in Russian](https://ap-pro.ru/uploads/monthly_2026_09/CB-WSN-179-21008.png.df864990619ce709f3fb8bcc3dfd62e0.png)

</details>

<details>
<summary>Supporting the bridge application</summary>

If you would like to support the author of the bridge application:

**T-Bank card:** `2200 3961 2613 9183`

This support request concerns the independently developed streaming application, its API integration, assets, and scripts — not the third-party game materials used by the mod.

</details>

</details>

<a id="original-map-sources"></a>
<details>
<summary><strong>Original map sources</strong></summary>

With [Artyom Bushuev's permission](https://vk.ru/artembushuev), the original map author's SDK resources are linked below. The cloud storage belongs to him.

**These are older map sources, from before later bug fixes. They do not match the map used by Living Zone or the current Chronicles of Pripyat build.**

- [my_pripyat211125.7z](https://disk.yandex.ru/d/h25r6YXZjw6TCQ)
- [textures07082016.7z](https://disk.yandex.ru/d/qC_moLkVHy4QRQ)
- [maps.zip](https://disk.yandex.ru/d/GiIZ9fiykhFf4g)
- [SDKCSFull.7z](https://disk.yandex.ru/d/J5UN4P5mDvG0dQ)

The project's separately published engine snapshot is available in the [engine-source branch](https://github.com/hardcoon/chronicles-of-pripyat-ixray/tree/engine-source).

</details>

<a id="troubleshooting"></a>
<details>
<summary><strong>Troubleshooting</strong></summary>

Start with the current launcher and game build. Old fixes copied from forum replies can interfere with newer releases; do not install a temporary fix unless it applies to your version.

<details>
<summary>Out of memory</summary>

Keep the Windows page file enabled and system-managed, leave enough free space on the system drive, close memory-heavy applications, and reduce graphics settings. The project guide recommends substantial system-drive headroom; a page file cannot grow if the drive is full.

If crashes continue, report your build, RAM, page-file configuration, settings, and the full game log from `userdata/logs`. There is no universal settings recipe or restart utility that guarantees to eliminate memory-related crashes.

</details>

<details>
<summary>The game crashes after installation or an update</summary>

Check that you are using the current launcher and build. Use **Verify files** to identify missing or altered managed files, then include its result and the game log in your report. Verification does not automatically repair the installation, and **Check for updates** does not run an integrity scan when no update is available.

If you installed third-party addons or temporary fixes, mention them in the report. A successful update does not mean that unrelated files you added have been removed.

</details>

<details>
<summary>A stash is inside a cabinet I cannot open</summary>

Use the starting metal detector supplied by the guide. It lets you access a nearby eligible stash that is difficult to reach by hand; the higher-tier stash detectors have a different purpose.

</details>

<details>
<summary>Orders are unavailable</summary>

Visit the checkpoint and guide straight ahead from the new-game starting area. Checkpoint-related progression must be opened before you can take orders.

</details>

<details>
<summary>Why are Zaton and the Stalker Bar sparsely populated?</summary>

The project was developed without current SDK sources for this map, so many additions rely on scripts and workarounds. The sources linked above are older and do not remove that limitation.

A sparsely populated Zone is also deliberate. I want to preserve abandonment, solitude, and danger. Important characters should have a reason to be there — a role, a story, a useful service, or something for the player to do. I am not filling every empty corner with anonymous NPCs just so someone is standing there. An empty place is not automatically an unfinished place.

</details>

</details>

<a id="community-addons-and-player-creations"></a>
<details>
<summary><strong>Community addons and player creations</strong></summary>

These are community contributions, not a promise of compatibility with every game build. Read each author's instructions and version requirements before installing.

| Contribution | Author / source |
| --- | --- |
| [Player-made interiors and sharing](https://ap-pro.ru/forums/topic/15559-chronicles-of-pripyat-gotovye-interery/) | Community collection |
| [Updated PDA map](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/4/?tab=comments#comment-562777) | segudei |
| [IX-Ray HD HUD](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=566058) | segudei |
| [Vehicle-stat rebalance](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=569474) | руская |
| [Vehicle rebalance](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=573304) | Яррик |
| [Summer vegetation](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=576954) | Blackbird |
| [H8 Classic Pack — NPC models](https://ap-pro.ru/forums/topic/15499-h8-classic-pack-modeley-dlya-stalker-zov-pripyati/) | See the pack's thread |

The forum installation example for H8 uses `ixr_addons/~h8-classic-pack`. Check compatibility with your current build rather than assuming that an older “no adaptation needed” note still applies.

Files you add yourself may survive launcher updates. **Verify files** can report differences in managed files, but it does not certify third-party addons or automatically remove them. Keep track of your own changes.

</details>

<a id="credits"></a>
<details>
<summary><strong>Credits</strong></summary>

Special thanks to the **IX-Ray developers** and the author of **Living Zone 4.26**.

Materials and work used in the project include:

- Living Zone 4.26.
- Call of Chernobyl (I.W.P.), including the repair-kit animations.
- Dead Air.
- Lost Alpha Definitive Car Pack Addon.
- Lost Alpha Enhanced Edition.
- NLC 3.9.1.
- Anomaly Radios Extended 1.5.1.
- Ambient Extended Reworked 1.1 — ambient soundtrack selection.
- EnVys Emisson Radio Rework V 1.1 — emission radio messages.
- S2HUD New CLASSIC 12.03.
- PDA Hacking.
- S2 Heart Of Chornobyl Backpack Anim.
- Hideout Furniture (HF) v2.3.2.
- Hideout Furniture Expansion.
- Even More Hideout Furniture [Update 2].
- [Secret Trails OGSR](https://ap-pro.ru/forums/topic/820-secret-trails/) — wristwatch.

Music selection: **MoozE**. Main-menu track: **Chikoi The Maid — Reflection**.

</details>

<a id="official-sources"></a>
<details>
<summary><strong>Official sources</strong></summary>

The two official project pages are:

- [The author's AP-PRO thread](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/).
- [hardcoon/chronicles-of-pripyat-ixray on GitHub](https://github.com/hardcoon/chronicles-of-pripyat-ixray).

The launcher also uses the project's Gitea download mirror. I do not run a project Discord, Telegram, or VK channel, and I do not maintain uploads on other sites. The VK links above belong to the original map author, not to a separate Chronicles community.

Use the official links instead of unknown repacks. AMK and ModDB were exceptions to my general warning about third-party sites, but that does not make an upload there an official build.

</details>

<a id="patch-history"></a>
<details>
<summary><strong>Patch history</strong></summary>

- [2026.11.21-dev.115](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=581797) — Route-based travel and a 3D skybox
- [2026.11.20-dev.114](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=581600)
- [2026.11.19-dev.113](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=581487) — Winter addon
- [2026.11.19-dev.112](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=581378)
- [2026.11.14-dev.110](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=580388) — Automatic in-chat translation
- [2026.11.09-dev.109](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=579265)
- [2026.11.06-dev.108](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=578554) — 3D PDA
- [2026.11.01-dev.107](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=577085)
- [2026.10.30-dev.105](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=576880)
- [2026.10.29-dev.103](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=576571)
- [2026.10.27-dev.102](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=576333) — Exoskeleton display and chat avatars
- [2026.10.26-dev.101](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=576143)
- [2026.10.26-dev.100](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=575939) — Main-story update
- [2026.10.25-dev.98](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=575685)
- [2026.10.23-dev.95](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=575544)
- [2026.10.23-dev.94](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=575380) — Player statistics
- [2026.10.20-dev.93](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=574966)
- [2026.10.19-dev.91](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=574744)
- [2026.10.18-dev.90](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=574589) — Secret Trails OGSR wristwatch
- [2026.10.17-dev.89](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=574218)
- [2026.10.16-dev.87](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=574037)
- [2026.10.15-dev.86](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=573874)
- [2026.10.14-dev.84](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=573504)
- [2026.10.14-dev.83](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=573412) — Sharing player-made interiors
- [2026.10.12-dev.80](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=573066) — SSS puddles
- [2026.10.10-dev.79](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=572633) — Photographs and workbench dismantling
- [2026.10.09-dev.78](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=572073) — Pseudo A-Life, campfires, and cooking
- [2026.10.07-dev.77](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=571779)
- [2026.10.07-dev.76](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=571585)
- [2026.10.06-dev.75](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=571225)
- [2026.10.05-dev.73](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=571036)
- [2026.10.05-dev.72](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/24/?tab=comments#comment-570935) — Furniture and workbench
- [2026.10.04-dev.70](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=570702) — Tank 1990
- [2026.09.28-dev.69](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/22/?tab=comments#comment-569451)
- [2026.09.27-dev.67](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/21/?tab=comments#comment-568951)
- [2026.09.25-dev.66](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=568478)
- [2026.09.24-dev.65](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/20/?tab=comments#comment-568425) — Interior pseudo-HDR
- [2026.09.23-dev.59](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/19/?tab=comments#comment-568200)
- [2026.09.21-dev.58](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/18/?tab=comments#comment-567675)
- [2026.09.20-dev.56](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/17/?tab=comments#comment-567390)
- [2026.09.19-dev.55](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=567199)
- [2026.09.18-dev.53](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/16/?tab=comments#comment-566862)
- [2026.09.16-dev.52](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/15/?tab=comments#comment-566131)
- [2026.09.12-dev.51](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/13/?tab=comments#comment-565169)
- [2026.09.11-dev.50](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/page/13/?tab=comments#comment-564484)
- [2026.09.10-dev.46](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=564249)
- [2026.09.08-dev.44](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=564137)
- [2026.09.08-dev.43](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563805)
- [2026.09.08-dev.41](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563689)
- [2026.09.08-dev.40](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563686)
- [2026.09.07-dev.39](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563663)
- [2026.09.07-dev.38](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563494) — Renderer optimization settings
- [2026.09.07-dev.37](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563325)
- [2026.09.06-dev.33](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563289)
- [2026.09.06-dev.32](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563168)
- [2026.09.05-dev.30](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=563017)
- [2026.09.05-dev.29](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=562931)
- [2026.09.05-dev.28](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=562787)
- [2026.09.04-dev.25](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=562504)
- [2026.09.02-dev.24](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=562173)
- [2026.09.01-dev.20](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=561750) — PDA minigames
- [2026.08.31-dev.18](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=560602)
- [2026.08.28-dev.3](https://ap-pro.ru/forums/topic/15193-chronicles-of-pripyat/?do=findComment&comment=560560)
- Original release: **August 27, 2026**.

</details>

## Legal notice

Chronicles of Pripyat is an unofficial, noncommercial fan modification for S.T.A.L.K.E.R. It is not affiliated with or endorsed by GSC Game World. S.T.A.L.K.E.R. and the original game materials belong to GSC Game World and their respective rights holders; third-party materials belong to their authors.

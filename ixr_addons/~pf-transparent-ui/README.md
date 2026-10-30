# Transparent inventory interface

Optional copy of the current inventory, looting, trading and repair layouts.
Enable **TRANSPARENT INTERFACE** in **MOD OPTIONS** and apply the settings.
The default is the original interface. The choice is stored in
`userdata/pf_transparent_interface.setting`, independently of saved games.

Requires the project engine patch `COP-ENG-209`, revision `eng-209-r2` or later.
This revision reads newly created client preferences outside the VFS cache.
Applying the checkbox uses
IX-Ray's existing `ui_reload` command; the next inventory window uses the
selected layout. Startup and resolution changes use the same choice.

The base XML covers 4:3 and 5:4, `_16` covers 16:9 and 16:10, and `_21`
covers ultrawide screens through IX-Ray's existing resolution selection.
Each layout retains all coordinates and widgets from its active original.
The wide copy includes the active inventory-warning layout.

All new background textures have separate names. Their RGB comes from the
existing inventory assets; only alpha changes. The opacity and inner-edge
shading come from the upper dialogue panel in
`ui_pf_inventory_talk_panel_16.dds`, with a small increase in darkness.
Panel gutters have zero alpha, and the original item icons, text, bars,
button actions, hover states and inventory behavior are retained.

Panel shading uses one continuous opacity field, including short equipment
and quick-slot panels. The trade overlay fades at both ends inside the side
panels so it does not cover their existing metal borders.
Trade price and weight keep a separate dark backing with softly blurred edges.

The original inventory XML, DDS textures, dialogue and other settings are
retained. Removing this addon makes the engine fall back to the original
layout even if the client preference is still enabled.

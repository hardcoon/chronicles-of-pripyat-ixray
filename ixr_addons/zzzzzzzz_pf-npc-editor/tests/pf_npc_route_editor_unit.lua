-- Pure Lua 5.1 checks for route schema/order. Run with:
-- luajit tests/pf_npc_route_editor_unit.lua scripts/pf_npc_route_editor.script

assert(arg[1], "route module path is required")

function class(name)
	return function()
		local result = {}
		result.__index = result
		_G[name] = result
		return result
	end
end

CUIListBoxItem = {}
CUIScriptWnd = {}
game = { translate_string = function(id) return id end }
level = {
	object_by_id = function() return nil end,
	name = function() return "unit_level" end,
}

dofile(arg[1])

local implementation
for index = 1, 20 do
	local name, value = debug.getupvalue(owns_npc, index)
	if not name then break end
	if name == "impl" then implementation = value break end
end
assert(implementation, "route implementation upvalue is unavailable")

local records = {}
local adapter = {
	feature_enabled = function() return false end,
	get_records = function() return records end,
	put_record = function(record) records[tostring(record.id)] = record end,
	setting_number = function(_, fallback) return fallback end,
	setting_string = function(_, fallback) return fallback end,
	report = function() end,
}

local legacy = {
	id = 7,
	level = "unit_level",
	route = {
		status = "unknown",
		points = {
			{ x = 1, y = 2, z = 3, wait_seconds = -9, wait_mode = "old" },
		},
	},
}
local migrated = implementation.route(legacy, false)
assert(migrated.status == "paused")
assert(migrated.points[1].index == 1)
assert(migrated.points[1].wait_seconds == 0.1)
assert(migrated.points[1].wait_mode == "time")
assert(owns_npc(legacy) == true)

for index = 2, 6 do
	migrated.points[index] = { x = index, y = 0, z = 0, wait_seconds = 5 }
end
migrated.shuffle = true
math.randomseed(426)
local order = implementation.build_order(migrated)
assert(order[1] == 1)
local seen = {}
for _, point_index in ipairs(order) do
	assert(point_index >= 1 and point_index <= #migrated.points)
	assert(not seen[point_index], "shuffle repeated a point")
	seen[point_index] = true
end
assert(#order == #migrated.points)

records[tostring(legacy.id)] = legacy
migrated.status = "walking"
update(adapter, 100)
assert(migrated.status == "paused", "disabled feature did not pause route")

migrated.execution_order = order
migrated.execution_pos = #order
migrated.current_point = order[#order]
migrated.loop = false
migrated.status = "waiting"
implementation.advance(legacy, migrated)
assert(migrated.status == "completed")
assert(migrated.current_point == order[#order])

print("pf_npc_route_editor_unit: OK")

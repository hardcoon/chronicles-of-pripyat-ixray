local intercepts =
{
	save = {},
	load = {},
	update = {},

	save_state = {},
	load_state = {}
}

-- Opt-in timed update callbacks. The existing callback lists keep their
-- original cadence; timed functions are invoked only when their deadline is due.
local timed_updates = {}
local timed_count, timed_revision = 0, 0
local timed_next, timed_last

local function remove_timed_update(func)
	if timed_updates[func] then
		timed_updates[func] = nil
		timed_count = timed_count - 1
		timed_revision = timed_revision + 1
		timed_next = nil
	end
end

function RegisterScriptCallback(name, func_or_userdata)
	if (func_or_userdata == nil) then
		SemiLog("! func_or_userdata == nil")
		callstack()
	end

	if (name == nil) then
		SemiLog("! name == nil")
		callstack()
	end
	if (intercepts == nil) then
		SemiLog("! intercepts == nil")
		callstack()
	end

	if (intercepts[name]) then
		if name == "update" then remove_timed_update(func_or_userdata) end
		intercepts[name][func_or_userdata] = true
	end
end

function RegisterScriptTimedCallback(func, interval_ms)
	assert(type(func) == "function", "Timed callback must be a function")
	assert(type(interval_ms) == "number" and interval_ms == interval_ms and
		interval_ms > 0 and interval_ms < math.huge, "Invalid callback interval")
	intercepts.update[func] = nil
	if not timed_updates[func] then timed_count = timed_count + 1 end
	timed_updates[func] = {interval = interval_ms}
	timed_revision = timed_revision + 1
	timed_next = nil
end

function UnregisterScriptCallback(name, func_or_userdata)
	if name == "update" then remove_timed_update(func_or_userdata) end
	if (intercepts[name]) then
		intercepts[name][func_or_userdata] = nil
	end
end

function SendScriptCallback(name,...)
	if (intercepts[name]) then
		for func_or_userdata,v in pairs(intercepts[name]) do
			if (type(func_or_userdata) == "function") then
				func_or_userdata(...)
			elseif (func_or_userdata[name]) then
				func_or_userdata[name](func_or_userdata,...)
			end
		end
	end
	if name == "update" and timed_count > 0 then
		local now = time_global()
		if timed_last and now < timed_last then
			-- Native time can restart after loading or wrap after a long session.
			for _, timer in pairs(timed_updates) do timer.due = nil end
			timed_next = nil
		end
		timed_last = now
		if timed_next and now < timed_next then return end
		local revision, next_due = timed_revision, nil
		for func, timer in pairs(timed_updates) do
			if not timer.due or now >= timer.due then
				-- Schedule before invoking, so reentrant dispatch cannot fire twice.
				-- A long frame produces one call, never a burst of missed updates.
				timer.due = now + timer.interval
				func(...)
			end
			if timed_updates[func] == timer then
				next_due = next_due and math.min(next_due, timer.due) or timer.due
			end
		end
		-- Registration may change inside a callback; reconsider it next dispatch.
		timed_next = revision == timed_revision and next_due or nil
	end
end

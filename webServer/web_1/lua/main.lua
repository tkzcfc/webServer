
_MyG = {}

local function main()
	require("logic")
end

__G__TRACKBACK__ = function(msg)
    local msg = debug.traceback(msg, 3)
    print(msg)
    return msg
end

local status, msg = xpcall(main, __G__TRACKBACK__)
if not status then
   -- print(msg)
end

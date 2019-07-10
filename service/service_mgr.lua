local mtask = require "mtask"
require "mtask.manager"	-- import mtask.register
local snax = require "mtask.snax"

local cmd = {}
local service = {}
-- 这里的 func 为 mtask.newservice
local function request(name, func, ...)
	local ok, handle = pcall(func, ...)
	local s = service[name]
	assert(type(s) == "table")
	if ok then
		service[name] = handle
	else
		service[name] = tostring(handle)
	end
	-- 唤醒阻塞的协程
	for _,v in ipairs(s) do
		mtask.wakeup(v)
	end

	if ok then
		return handle
	else
		error(tostring(handle))
	end
end
-- 惰性初始化
-- 如果是首次调用会直接返回
local function waitfor(name , func, ...)
	local s = service[name]
	-- 如果已经有了，就直接返回
	if type(s) == "number" then
		return s
	end
	local co = coroutine.running()

	if s == nil then
		s = {}
		service[name] = s
	elseif type(s) == "string" then
		error(s)
	end

	assert(type(s) == "table")

    -- 如果是首次调用,可以直接返回
    local session, source = mtask.context()

    if s.launch == nil and func then
        s.launch = {
        session = session,
        source = source,
        co = co,
    }
    return request(name, func, ...)
    end

    table.insert(s, {
        co = co,
        session = session,
        source = source,
    })
    mtask.wait()
	s = service[name]
	if type(s) == "string" then
		error(s)
	end
	assert(type(s) == "number")
	return s
end

local function read_name(service_name)
	if string.byte(service_name) == 64 then -- 64的ASCII码为 '@'
		return string.sub(service_name , 2)
	else
		return service_name
	end
end

function cmd.LAUNCH(service_name, subname, ...)
	local realname = read_name(service_name)

	if realname == "snaxd" then
		return waitfor(service_name.."."..subname, snax.rawnewservice, subname, ...)
	else
		return waitfor(service_name, mtask.newservice, realname, subname, ...)
	end
end

function cmd.QUERY(service_name, subname)
	local realname = read_name(service_name)

	if realname == "snaxd" then
		return waitfor(service_name.."."..subname)
	else
		return waitfor(service_name)
	end
end

local function list_service()
	local result = {}
	for k,v in pairs(service) do
		if type(v) == "string" then
			v = "Error: " .. v
		elseif type(v) == "table" then
            local querying = {}
            if v.launch then
                local session = mtask.task(v.launch.co)
                local launching_address = mtask.call(".launcher", "lua", "QUERY", session)
                if launching_address then
                    table.insert(querying, "Init as " .. mtask.address(launching_address))
                    table.insert(querying,  mtask.call(launching_address, "debug", "TASK", "init"))
                    table.insert(querying, "Launching from " .. mtask.address(v.launch.source))
                    table.insert(querying, mtask.call(v.launch.source, "debug", "TASK", v.launch.session))
                end
            end
            if #v > 0 then
                table.insert(querying , "Querying:" )
                for _, detail in ipairs(v) do
                    table.insert(querying, mtask.address(detail.source) .. " " .. tostring(mtask.call(detail.source, "debug", "TASK", detail.session)))
                end
            end
            v = table.concat(querying, "\n")
		else
			v = mtask.address(v)
		end

		result[k] = v
	end

	return result
end

-- 单节点模式、多节点中的主节点
local function register_global()
	function cmd.GLAUNCH(name, ...)
		local global_name = "@" .. name
		return cmd.LAUNCH(global_name, ...)
	end

	function cmd.GQUERY(name, ...)
		local global_name = "@" .. name
		return cmd.QUERY(global_name, ...)
	end

	local mgr = {}

	function cmd.REPORT(m)
		mgr[m] = true
	end

	local function add_list(all, m)
		local harbor = "@" .. mtask.harbor(m)
		local result = mtask.call(m, "lua", "LIST")
		for k,v in pairs(result) do
			all[k .. harbor] = v
		end
	end

	function cmd.LIST()
		local result = {}
		for k in pairs(mgr) do
			pcall(add_list, result, k)
		end
		local l = list_service()
		for k, v in pairs(l) do
			result[k] = v
		end
		return result
	end
end
-- 多节点中的非主节点
local function register_local()
	local function waitfor_remote(cmd, name, ...)
		local global_name = "@" .. name
		local local_name
		if name == "snaxd" then
			local_name = global_name .. "." .. (...)
		else
			local_name = global_name
		end
		-- 注意此时的 func变为 mtask.call 了， cmd为 "LAUNCH"
		return waitfor(local_name, mtask.call, "SERVICE", "lua", cmd, global_name, ...)
	end

	function cmd.GLAUNCH(...)
		return waitfor_remote("LAUNCH", ...)
	end

	function cmd.GQUERY(...)
		return waitfor_remote("QUERY", ...)
	end

	function cmd.LIST()
		return list_service()
	end

	mtask.call("SERVICE", "lua", "REPORT", mtask.self())
end

mtask.start(function()
	mtask.dispatch("lua", function(session, address, command, ...)
		local f = cmd[command]
		if f == nil then
			mtask.ret(mtask.pack(nil, "Invalid command " .. command))
			return
		end

		local ok, r = pcall(f, ...)

		if ok then
			mtask.ret(mtask.pack(r))
		else
			mtask.ret(mtask.pack(nil, r))
		end
	end)
	local handle = mtask.localname ".service"--返回"service"的数字地址
	if  handle then
		mtask.error(".service is already register by ", mtask.address(handle))
		mtask.exit()
	else
		mtask.register(".service")
	end
	if mtask.getenv "standalone" then -- 主节点,单节点模式
		mtask.register("SERVICE")
		register_global()
	else
		register_local() -- 多节点模式中的非主节点
	end
end)

local mtask = require "mtask"

local clusterd
local cluster = {}
local sender = {}
local task_queue = {}

local function request_sender(q, node)
	local ok, c = pcall(mtask.call, clusterd, "lua", "sender", node)
	if not ok then
		mtask.error(c)
		c = nil
	end
	-- run tasks in queue
	local confirm = coroutine.running()
	q.confirm = confirm
	q.sender = c
	for _, task in ipairs(q) do
		if type(task) == "table" then
			if c then
				mtask.send(c, "lua", "push", task[1], mtask.pack(table.unpack(task,2,task.n)))
			end
		else
			mtask.wakeup(task)
			mtask.wait(confirm)
		end
	end
	task_queue[node] = nil
	sender[node] = c
end

local function get_queue(t, node)
	local q = {}
	t[node] = q
	mtask.fork(request_sender, q, node)
	return q
end

setmetatable(task_queue, { __index = get_queue } )

local function get_sender(node)
	local s = sender[node]
	if not s then
		local q = task_queue[node]
		local task = coroutine.running()
		table.insert(q, task)
		mtask.wait(task)
		mtask.wakeup(q.confirm)
		return q.sender
	end
	return s
end

function cluster.call(node, address, ...)
	-- mtask.pack(...) will free by cluster.core.packrequest
	return mtask.call(get_sender(node), "lua", "req", address, mtask.pack(...))
end

function cluster.send(node, address, ...)
	-- push is the same with req, but no response
	local s = sender[node]
	if not s then
		table.insert(task_queue[node], table.pack(address, ...))
	else
		mtask.send(sender[node], "lua", "push", address, mtask.pack(...))
	end
end

function cluster.open(port)
	if type(port) == "string" then
		mtask.call(clusterd, "lua", "listen", port)
	else
		mtask.call(clusterd, "lua", "listen", "0.0.0.0", port)
	end
end

function cluster.reload(config)
	mtask.call(clusterd, "lua", "reload", config)
end

function cluster.proxy(node, name)
	return mtask.call(clusterd, "lua", "proxy", node, name)
end

function cluster.snax(node, name, address)
	local snax = require "mtask.snax"
	if not address then
		address = cluster.call(node, ".service", "QUERY", "snaxd" , name)
	end
	local handle = mtask.call(clusterd, "lua", "proxy", node, address)
	return snax.bind(handle, name)
end

function cluster.register(name, addr)
	assert(type(name) == "string")
	assert(addr == nil or type(addr) == "number")
	return mtask.call(clusterd, "lua", "register", name, addr)
end

function cluster.query(node, name)
	-- 注意第5个参数为0
	return mtask.call(clusterd, "lua", "req", node, 0, mtask.pack(name))
end

mtask.init(function()
	clusterd = mtask.uniqueservice("clusterd")
end)

return cluster

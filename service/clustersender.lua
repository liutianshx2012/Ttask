local mtask = require "mtask"
local sc = require "mtask.socketchannel"
local socket = require "mtask.socket"
local cluster = require "mtask.cluster.core"

local channel
local session = 1
local node, nodename = ...

local command = {}
local waiting = {}


local function send_request(addr, msg, sz)
	-- msg is a local pointer, cluster.packrequest will free it
	local current_session = session
	local request, new_session, padding = cluster.packrequest(addr, session, msg, sz)
	session = new_session

	local tracetag = mtask.tracetag()
	if tracetag then
		if tracetag:sub(1,1) ~= "(" then
			-- add nodename
			local newtag = string.format("(%s-%s-%d)%s", nodename, node, session, tracetag)
			mtask.tracelog(tracetag, string.format("session %s", newtag))
			tracetag = newtag
		end
		mtask.tracelog(tracetag, string.format("cluster %s", node))
		channel:request(cluster.packtrace(tracetag))
	end
	return channel:request(request, current_session, padding)
end

local function wait()
	local co = coroutine.running()
	table.insert(waiting, co)
	mtask.wait(co)
end

function command.req(...)
	if channel == nil then
		wait()
	end
	local ok, msg = pcall(send_request, ...)
	if ok then
		if type(msg) == "table" then
			mtask.ret(cluster.concat(msg))
		else
			mtask.ret(msg)
		end
	else
		mtask.error(msg)
		mtask.response()(false)
	end
end

function command.push(addr, msg, sz)
	if channel == nil then
		wait()
	end
	local request, new_session, padding = cluster.packpush(addr, session, msg, sz)
	if padding then	-- is multi push
		session = new_session
	end

	channel:request(request, nil, padding)
end

local function read_response(sock)
	local sz = socket.header(sock:read(2))
	local msg = sock:read(sz)
	return cluster.unpackresponse(msg)	-- session, ok, data, padding
end

function command.changenode(host, port)
	local c = sc.channel {
			host = host,
			port = tonumber(port),
			response = read_response,
			nodelay = true,
		}
	local succ, err = pcall(c.connect, c, true)
	if channel then
		channel:close()
	end
	if succ then
		channel = c
		for k, co in ipairs(waiting) do
			waiting[k] = nil
			mtask.wakeup(co)
		end
		mtask.ret(mtask.pack(nil))
	else
		channel = nil	-- reset channel
		mtask.response()(false)
	end
end

mtask.start(function()
	mtask.dispatch("lua", function(session , source, cmd, ...)
		local f = assert(command[cmd])
		f(...)
	end)
end)

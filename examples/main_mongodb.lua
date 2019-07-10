local mtask = require "mtask"


mtask.start(function()
	print("Main Server start")
	local console = mtask.newservice(
		"testmongodb", "127.0.0.1", 27017, "testdb", "test", "test"
	)
	
	print("Main Server exit")
	mtask.exit()
end)

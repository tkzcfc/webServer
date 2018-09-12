serverInstance:register_call(GET, "/api/hello", function(svr, connection, http_msg)
	svr:send_response(connection, "welcome to httpserver");
end)

serverInstance:register_call(POST, "/api/sum", function(svr, connection, http_msg)
	--简单post请求，加法运算测试
	local n1 = get_http_var(http_msg.body, "n1")
	local n2 = get_http_var(http_msg.body, "n2")
	n1 = tonumber(n1)
	n2 = tonumber(n2)

	local ret = 0
	if type(n1) == "number" and type(n2) == "number" then
		ret = n1 + n2
	end
	local str = string.format([[{ "result": %d }]], ret)
	svr:send_response(connection, str);
	print(str)
end)

serverInstance:register_call(GET, "/api/hello", function(svr, connection, http_msg)
	svr:send_response(connection, "hello");
end)

serverInstance:register_call(GET, "/api/close", function(svr, connection, http_msg)
	local password = get_http_var(http_msg.query_string, "password")
	local username = get_http_var(http_msg.query_string, "username")
	if username == "admin" and password == "test" then
		svr:unregister_all()
	else
		svr:send_response(connection, "invalid param");
	end
end)


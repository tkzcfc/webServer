#pragma once

#include "webServerDef.h"
#include <map>
#include <functional>
#include <thread>
#include "LuaFunction.h"




enum METHOD
{
	GET,
	PUT,
	POST,
	ALL
};

class webServer;
typedef std::function<void(webServer*, mg_connection*, http_message*)> call_func;

struct web_server_config
{
	std::string port;
	bool enable_directory_listing;
	std::string web_root;
	std::string lua_root;
	std::string lua_start_file;
	std::string server_name;
};

class webServer
{
public:
	webServer();
	~webServer();

	bool start(const std::string& work_dir, const std::function<void(lua_State*)>& register_call = nullptr);

	bool restart();

	void stop();

	bool register_call(METHOD method, const std::string& uri, const LuaFunction& call);

	bool register_call(METHOD method, const std::string& uri, const call_func& call);

	void unregister_call(METHOD method, const std::string& uri);

	void unregister_all();

	void send_response(mg_connection *connection, const std::string& response);

	METHOD get_method(http_message *http_req);

	void print_log(const std::string& logstr);

	void goto_web(mg_connection* connection, http_message* http_req);

	inline web_server_config* get_config() { return m_config; }

	inline bool isstop() { return !m_start; }

	bool isinvalid() { return m_isinvalid; }

	const std::string& get_server_tag();

	inline mg_connection* get_connection() { return m_connection; }

	inline mg_mgr* get_mgr() { return m_mgr; }

protected:

	bool loadconfig(const std::string& config_path);

	bool update_server_parameters();

	void do_server_poll();

	void on_http_event(mg_connection *connection, int event_type, void *event_data);

	void on_http_request(mg_connection *connection, http_message *http_req);

	void clear();

	// 启动lua虚拟机
	bool start_lua_state();

public:
	mg_serve_http_opts m_server_option; // web服务器选项
	mg_mgr* m_mgr;						// 连接管理器
	mg_connection* m_connection;		// 链接
protected:
	std::map<std::string, call_func > m_calls[METHOD::ALL + 1];
	std::map<std::string, LuaFunction* > m_lua_calls[METHOD::ALL + 1];
	
	lua_State* m_stateL;
	std::thread* m_thread;
	web_server_config* m_config;

	std::string m_work_dir;
	std::function<void(lua_State*)> m_register_call;

	std::string m_tag_cache;

	bool m_start;
	bool m_isinvalid;
private:
	static void g_on_http_event(mg_connection *connection, int event_type, void *event_data);
};

#include "webServer.h"
#include "tools.h"
#include "log.h"
#include "LuaInlayCode.h"
#include "web_tolua.h"

extern "C"
{
	#include "lfs/lfs.h"
}

static const char* method_name[METHOD::ALL] = {
	"GET",
	"PUT",
	"POST",
};

webServer::webServer()
	: m_connection(NULL)
	, m_mgr(NULL)
	, m_stateL(NULL)
	, m_config(NULL)
	, m_thread(NULL)
	, m_start(false)
	, m_isinvalid(false)
{}

webServer::~webServer()
{
	stop();
	if (m_config != NULL)
	{
		delete m_config;
		m_config = NULL;
	}
}

bool webServer::start(const std::string& work_dir, const std::function<void(lua_State*)>& register_call)
{
	if (m_start)
		return false;
	m_isinvalid = false;
	m_work_dir = work_dir;
	tools::inspect_dir_path(m_work_dir);

	std::string config_path = m_work_dir + "config.lua";
	if (!loadconfig(config_path))
	{
		delete m_config;
		m_config = NULL;
		return false;
	}

	printf("starting http server at port: %s\n", m_config->port.c_str());
	m_start = true;
	m_register_call = register_call;

	m_thread = new std::thread([this]() { this->do_server_poll(); });

	return true;
}

bool webServer::restart()
{
	this->stop();
	return this->start(m_work_dir, m_register_call);
}

void webServer::stop()
{
	if (!m_start)
	{
		return;
	}
	m_start = false;
	m_thread->join();
	delete m_thread;
	m_thread = NULL;
}

bool webServer::loadconfig(const std::string& config_path)
{
	if (m_config != NULL)
	{
		delete m_config;
		m_config = NULL;
	}
	m_config = new web_server_config();

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_lfs(L);

	const char* lua_start_script =
		"local lfs = require(\"lfs\")"
		"currentdir = lfs.currentdir()";
	luaL_dostring(L, lua_start_script);

	lua_getglobal(L, "currentdir");
	std::string cur_dir = lua_tostring(L, -1);
	tools::inspect_dir_path(cur_dir);

	cur_dir.append(m_work_dir);
	cur_dir.pop_back();

	lua_pushstring(L, cur_dir.c_str());
	lua_setglobal(L, "currentdir");

	if (luaL_dofile(L, config_path.c_str()) != 0)
	{
		fprintf(stderr, "ERROR:%s\n\n", lua_tostring(L, -1));
		return false;
	}

	// port
	lua_getglobal(L, "port");
	if (lua_isnumber(L, -1) == 0)
	{
		fprintf(stderr, "the port number does not exist");
		lua_close(L);
		return false;
	}
	m_config->port = std::to_string((int)lua_tonumber(L, -1));

	// web_root
	lua_getglobal(L, "web_root");
	if (lua_isstring(L, -1) == 0)
	{
		fprintf(stderr, "the 'web_root' does not exist");
		lua_close(L);
		return false;
	}
	m_config->web_root = lua_tostring(L, -1);

	// lua_root
	lua_getglobal(L, "lua_root");
	if (lua_isstring(L, -1) == 0)
	{
		fprintf(stderr, "the 'lua_root' does not exist");
		lua_close(L);
		return false;
	}
	m_config->lua_root = lua_tostring(L, -1);

	// lua_start_file
	lua_getglobal(L, "lua_start_file");
	if (lua_isstring(L, -1) == 0)
	{
		fprintf(stderr, "the 'lua_start_file' does not exist");
		lua_close(L);
		return false;
	}
	m_config->lua_start_file = lua_tostring(L, -1);

	// lua_start_file
	lua_getglobal(L, "server_name");
	if (lua_isstring(L, -1) == 0)
	{
		fprintf(stderr, "the 'server_name' does not exist");
		lua_close(L);
		return false;
	}
	m_config->server_name = lua_tostring(L, -1);
	
	// enable_directory_listing
	lua_getglobal(L, "enable_directory_listing");
	if (lua_isboolean(L, -1) == 0)
	{
		fprintf(stderr, "the 'enable_directory_listing' does not exist");
		lua_close(L);
		return false;
	}
	m_config->enable_directory_listing = lua_toboolean(L, -1);

	lua_close(L);

	if (m_config->web_root != ".")
	{
		tools::inspect_dir_path(m_config->web_root);
	}
	tools::inspect_dir_path(m_config->lua_root);
	
	if (m_config->web_root.empty())
	{
		m_config->web_root = ".";
	}
	return true;
}

bool webServer::update_server_parameters()
{
	memset(&m_server_option, 0, sizeof(m_server_option));
	m_server_option.enable_directory_listing = m_config->enable_directory_listing ? "yes" : "no";
	m_server_option.document_root = m_config->web_root.c_str();

	m_mgr = (mg_mgr*)malloc(sizeof(mg_mgr));
	mg_mgr_init(m_mgr, NULL);
	m_connection = mg_bind(m_mgr, m_config->port.c_str(), g_on_http_event);

	if (m_connection == NULL)
		return false;

	m_connection->user_data = this;
	mg_set_protocol_http_websocket(m_connection);

	return start_lua_state();
}

void webServer::do_server_poll()
{
	m_start = update_server_parameters();
	if (!m_start)
	{
		m_isinvalid = true;
		fprintf(stderr, "ERROR:start server [%s][%s] fail\n", m_config->server_name.c_str(), m_config->port.c_str());
	}
	while (m_start)
	{
		mg_mgr_poll(m_mgr, 100);
	}
	clear();
}

void webServer::clear()
{
	m_start = false;
	if (m_mgr)
	{
		mg_mgr_free(m_mgr);
		free(m_mgr);
		m_mgr = NULL;
	}
	unregister_all();
	if (m_stateL != NULL)
	{
		lua_close(m_stateL);
		m_stateL = NULL;
	}
	m_tag_cache.clear();
}

bool webServer::register_call(METHOD method, const std::string& uri, const LuaFunction& call)
{
	LuaFunction* pfunc = new LuaFunction(call);

	bool ret = register_call(method, uri, [=](webServer* svr, mg_connection* connection, http_message* http_req) {
		http_msg* msg = new http_msg(http_req);
		pfunc->ppush();
		pfunc->pushusertype(svr, "webServer");
		pfunc->pushusertype(connection, "mg_connection");
		pfunc->pushusertype(msg, "http_msg");
		pfunc->pcall();
		delete msg;
	});

	if (ret)
	{
		m_lua_calls[method].emplace(uri, pfunc);
	}
	else
	{
		delete pfunc;
	}
	return ret;
}

bool webServer::register_call(METHOD method, const std::string& uri, const call_func& call)
{
	if (method < 0 || method > METHOD::ALL)
	{
		return false;
	}
	std::map<std::string, call_func >* pcalls = &m_calls[method];
	auto it = pcalls->find(uri);
	if (it == pcalls->end())
	{
		pcalls->emplace(uri, call);
		return true;
	}
	return false;
}

void webServer::unregister_call(METHOD method, const std::string& uri)
{
	if (method < 0 || method > METHOD::ALL)
	{
		return;
	}
	std::map<std::string, call_func >* pcalls = &m_calls[method];
	auto it = pcalls->find(uri);
	if (it != pcalls->end())
	{
		pcalls->erase(it);
		auto lua_it = m_lua_calls[method].find(uri);
		if (lua_it != m_lua_calls[method].end())
		{
			delete lua_it->second;
			m_lua_calls[method].erase(lua_it);
		}
	}
}

void webServer::unregister_all()
{
	for (int i = 0; i <= METHOD::ALL; ++i)
	{
		for (auto it = m_lua_calls[i].begin(); it != m_lua_calls[i].end(); ++it)
		{
			delete it->second;
		}
		m_lua_calls[i].clear();
	}
	for (int i = 0; i <= METHOD::ALL; ++i)
	{
		m_calls[i].clear();
	}
}

void webServer::send_response(mg_connection *connection, const std::string& response)
{
	// 必须先发送header
	mg_printf(connection, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	
	if (!response.empty())
	{
		mg_printf_http_chunk(connection, response.c_str());
	}
	// 发送空白字符快，结束当前响应
	mg_send_http_chunk(connection, "", 0);
}

void webServer::on_http_event(mg_connection *connection, int event_type, void *event_data)
{
	http_message *http_req = (http_message *)event_data;
	switch (event_type)
	{
	case MG_EV_HTTP_REQUEST:
	{
		on_http_request(connection, http_req);
	}
		break;
	default:
		break;
	}
}

METHOD webServer::get_method(http_message *http_req)
{
	METHOD out = METHOD::ALL;
	for (int i = 0; i < METHOD::ALL; ++i)
	{
		if (mg_vcmp(&http_req->method, method_name[i]) == 0)
		{
			out = (METHOD)i;
			break;;
		}
	}
	return out;
}

const std::string& webServer::get_server_tag()
{
	if (!m_tag_cache.empty())
		return m_tag_cache;
	m_tag_cache = " [";
	m_tag_cache.append(m_config->server_name);
	m_tag_cache.append("(");
	m_tag_cache.append(m_config->port);
	m_tag_cache.append(")");
	m_tag_cache.append("] ");
	return m_tag_cache;
}

void webServer::print_log(const std::string& logstr)
{
	std::string output = get_server_tag();
	output.append(logstr);
	log(output.c_str());
}

void webServer::goto_web(mg_connection* connection, http_message* http_req)
{
	mg_serve_http(connection, http_req, m_server_option);
}

void webServer::on_http_request(mg_connection *connection, http_message *http_req)
{
	uint32_t remote_ip = (*(uint32_t *)&connection->sa.sin.sin_addr);
	uint8_t* p = (uint8_t*)&remote_ip;

	const std::string& stag = get_server_tag();
	log("%sIP:%hhu.%hhu.%hhu.%hhu", stag.c_str(), p[0], p[1], p[2], p[3]);

	std::string uri = std::string(http_req->uri.p, http_req->uri.len);
	std::map<std::string, call_func >* pcalls = NULL;
	std::map<std::string, call_func >::iterator it;

	METHOD method = get_method(http_req);

	if (method != METHOD::ALL)
	{
		pcalls = &m_calls[method];
		it = pcalls->find(uri);
		if (it != pcalls->end())
		{
			it->second(this, connection, http_req);
			return;
		}
		method = METHOD::ALL;
	}
	pcalls = &m_calls[method];
	it = pcalls->find(uri);
	if (it != pcalls->end())
	{
		it->second(this, connection, http_req);
		return;
	}

	mg_printf(
		connection,
		"%s",
		"HTTP/1.1 501 Not Implemented\r\n"
		"Content-Length: 0\r\n\r\n");
}

bool webServer::start_lua_state()
{
	lua_State* L = luaL_newstate();

	luaL_openlibs(L);
	luaopen_lfs(L);
	tolua_run_open(L);

	tolua_pushusertype(L, this, "webServer");
	lua_setglobal(L, "serverInstance");

	tolua_InlayCode_open(L);

	if (m_register_call != nullptr)
	{
		m_register_call(L);
	}
	if (m_config->lua_start_file.empty())
	{
		m_stateL = L;
		return true;
	}

	std::string fullpath = m_config->lua_root;
	fullpath.append("?.lua;");

	std::string start_fullpath = m_config->lua_root;
	start_fullpath.append(m_config->lua_start_file);

	lua_pushstring(L, fullpath.c_str());
	lua_setglobal(L, "__path");

	const char* lua_start_script =
		"package.path = package.path..';'..__path\n"
		"__path = nil\n";
	luaL_dostring(L, lua_start_script);

	if (luaL_dofile(L, start_fullpath.c_str()) != 0)
	{
		fprintf(stderr, "ERROR:%s\n\n", lua_tostring(L, -1));
		return false;
	}

	m_stateL = L;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void webServer::g_on_http_event(mg_connection *connection, int event_type, void *event_data)
{
	webServer* svr = (webServer*)connection->user_data;
	svr->on_http_event(connection, event_type, event_data);
}


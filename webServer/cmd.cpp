#include "cmd.h"
#include "webServer.h"
#include "log.h"
#include "lua-module/web_tolua.h"


std::map<int, webServer*> server_list;
int server_key_index = 0;

void start_new_server(const std::string& root_dir)
{
	webServer* svr = new webServer();

	if (svr == NULL)
	{
		log_print("start [%s] fail (out of memory)", root_dir.c_str());
		return;
	}

	bool suc = svr->start(root_dir, [](lua_State* L) {
		tolua_run_open(L);
	});

	if (!suc)
	{
		log_print("start [%s] fail (parameter error)", root_dir.c_str());
		delete svr;
		return;
	}
	server_list.insert(std::make_pair(server_key_index++, svr));
}

void stop_server(int key)
{
	auto it = server_list.find(key);
	if (it != server_list.end())
	{
		it->second->stop();
	}
	else
	{
		log_print("invalid server");
	}
}

void restart_server(int key)
{
	auto it = server_list.find(key);
	if (it != server_list.end() && it->second->isstop())
	{
		it->second->restart();
	}
	else
	{
		log_print("invalid server");
	}
}

void remove_server(int key)
{
	auto it = server_list.find(key);
	if (it != server_list.end())
	{
		it->second->stop();
		delete it->second;
		server_list.erase(it);
	}
	else
	{
		log_print("invalid server");
	}
}

void clear_invalid_server()
{
	for (auto it = server_list.begin(); it != server_list.end(); )
	{
		if (it->second->isinvalid())
		{
			delete it->second;
			it = server_list.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void print_server_list()
{
	log_print("\nkey\t-\t info\t status");
	for (auto& it : server_list)
	{
		if (it.second->get_config() == NULL || it.second->isinvalid())
		{
			log_print("%d\t-\t invalid server\t invalid", it.first);
		}
		else
		{
			std::string server_name = it.second->get_config()->server_name;
			server_name.append(":[");
			server_name.append(it.second->get_config()->port);
			server_name.append("]");
			log_print("%d\t-\t %s\t %s", it.first, server_name.c_str(), it.second->isstop() ? "stop" : "run");
		}
	}
	if (server_list.empty())
	{
		log_print("--\t-\t --\t --");
	}
}

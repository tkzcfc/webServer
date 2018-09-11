#include <iostream>
#include "webServer.h"
#include "tools.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lua-module/lfs/lfs.h"
}
#include "lua-module/web_tolua.h"
#include "log.h"
#include <thread>
#include "linenoise.h"
#include "cmd.h"

bool exit_main_thread = false;
bool exit_log_thread = false;

void log_thread_run()
{
	int len = 0;
	log_init("log/", 1024 * 2);
	log("start...");
	while (!exit_log_thread)
	{
		log_update();
		Sleep(50);
	}
	log("exit...");
	log_exit();
}

void cmd_resolve(const std::string& cmd);

int main(int argc, char *argv[])
{
	std::thread log_thread = std::thread(log_thread_run);
	
	const auto path = "history.txt";

	// Enable the multi-line mode
	linenoise::SetMultiLine(true);

	// Set max length of the history
	linenoise::SetHistoryMaxLen(6);

	// Setup completion words every time when a user types
	linenoise::SetCompletionCallback([](const char* editBuffer, std::vector<std::string>& completions) {
		if (editBuffer[0] == 'h') {
#ifdef _WIN32
			completions.push_back("hello こんにちは");
			completions.push_back("hello こんにちは there");
#else
			completions.push_back("hello");
			completions.push_back("hello there");
#endif
		}
	});

	// Load history
	linenoise::LoadHistory(path);

	while (!exit_main_thread) {
		std::string line;
		linenoise::Readline("> ", line);

		//auto quit = linenoise::Readline("> ", line);
		//if (quit) {
		//	break;
		//}

		cmd_resolve(line);
		
		// Add line to history
		linenoise::AddHistory(line.c_str());

		// Save history
		linenoise::SaveHistory(path);
	}

	exit_log_thread = true;
	log_thread.join();

	return 0;
}


void cmd_resolve(const std::string& cmd)
{
	if (cmd.empty())
		return;

	std::vector<std::string> params = tools::get_cmd_params(cmd);

	if (params.empty())
		return;
	if (params[0] == "clear" && params.size() > 1 && params[1] == "invalid")
	{
		clear_invalid_server();
	}
	else if (params[0] == "exit")
	{
		exit_main_thread = true;
	}
	else if (params[0] == "list")
	{
		print_server_list();
	}
	else if (params[0] == "new")
	{
		if (params.size() < 2)
		{
			log_print("workspace is nil");
			return;
		}
		start_new_server(params[1]);
	}
	else if (params[0] == "stop")
	{
		if (params.size() < 2)
		{
			log_print("server key is nil");
			return;
		}
		if (!tools::check_cmd_is_number(params[1]))
		{
			log_print("server key is invalid");
			return;
		}
		stop_server(std::atoi(params[1].c_str()));
	}
	else if (params[0] == "restart")
	{
		if (params.size() < 2)
		{
			log_print("server key is nil");
			return;
		}
		if (!tools::check_cmd_is_number(params[1]))
		{
			log_print("server key is invalid");
			return;
		}
		restart_server(std::atoi(params[1].c_str()));
	}
	else if (params[0] == "remove")
	{
		if (params.size() < 2)
		{
			log_print("server key is nil");
			return;
		}
		if (!tools::check_cmd_is_number(params[1]))
		{
			log_print("server key is invalid");
			return;
		}
		remove_server(std::atoi(params[1].c_str()));
	}
	else if (params[0] == "/?" || params[0] == "-h" || params[0] == "h")
	{
		const char* help =
			"\n\t- list\n"
			"\t- new \t[work_dir]\n"
			"\t- remove \t[key]\n"
			"\t- restart \t[key]\n"
			"\t- stop	\t[key]\n"
			"\t- clear invalid\n"
			"\t- exit\n";
		log_print(help);
	}
	else
	{
		system(cmd.c_str());
		//log_print("invalid param");
	}
}




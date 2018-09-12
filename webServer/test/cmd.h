#pragma once

#include <string>
#include <vector>

void start_new_server(const std::string& root_dir);

void stop_server(int key);

void restart_server(int key);

void remove_server(int key);

void clear_invalid_server();

void print_server_list();


std::vector<std::string> get_cmd_params(const std::string& cmd);

bool check_cmd_is_number(const std::string& cmd);
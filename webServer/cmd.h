#pragma once

#include <string>

void start_new_server(const std::string& root_dir);

void stop_server(int key);

void restart_server(int key);

void remove_server(int key);

void clear_invalid_server();

void print_server_list();

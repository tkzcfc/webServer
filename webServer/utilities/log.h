#pragma once

#include <string>

bool log_init(const char* root_dir, int max_size);

void log(const char* format, ...);

void log_print(const char* format, ...);

void log_update();

void log_exit();

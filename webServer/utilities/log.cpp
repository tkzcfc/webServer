#include "log.h"
#include <iostream>
#include <string>
#include <time.h>
#include <queue>
#include <stdarg.h>
#include <mutex>
#include <direct.h>

struct log_queue_block
{
	std::string content;
	bool issave;
};

FILE* logFP = NULL;
int log_file_size = 0;
int log_file_max_size = (1024 * 1024 * 4);
int log_cur_day = -1;
std::string log_root_dir = "log/";
std::queue<log_queue_block> log_write_queue;
std::mutex log_queue_mutex;
time_t log_create_file_time = 0;
int log_create_file_count = 0;

void log_create_dir(const char* dir_path)
{
#define MAX_PATH 256
	if (strlen(dir_path) > MAX_PATH)
	{
		return;
	}
	int ipathLength = strlen(dir_path);
	int ileaveLength = 0;
	int iCreatedLength = 0;
	char szPathTemp[MAX_PATH] = { 0 };
	for (int i = 0; (NULL != strchr(dir_path + iCreatedLength, '/')); i++)
	{
		ileaveLength = strlen(strchr(dir_path + iCreatedLength, '/')) - 1;
		iCreatedLength = ipathLength - ileaveLength;
		strncpy(szPathTemp, dir_path, iCreatedLength);
		mkdir(szPathTemp);
	}
	if (iCreatedLength < ipathLength)
	{
		mkdir(dir_path);
	}
}

std::string& log_string_replace_all(std::string& str, const std::string& old_value, const std::string& new_value)
{
	while (true) {
		std::string::size_type pos(0);
		if ((pos = str.find(old_value)) != std::string::npos)
			str.replace(pos, old_value.length(), new_value);
		else
			break;
	}
	return   str;
}

bool log_openfile(const char* filepath)
{
	log_exit();

	logFP = fopen(filepath, "ab+");
	
	if (logFP == NULL)
	{
		printf("open %s fail\n", filepath);
		return false;
	}

	fseek(logFP, 0L, SEEK_END);
	log_file_size = ftell(logFP);
	return true;
}

bool log_file_is_exist(const char* filename)
{
	if (filename == NULL)
		return false;

	FILE* fp = fopen(filename, "rb");
	if (fp == NULL)
		return false;
	fclose(fp);
	return true;
}

std::string log_new_file_name()
{
	time_t curtime = time(NULL);
	tm* localt = localtime(&curtime);
	
	static char timebuf[128];
	sprintf(timebuf, "%d-%02d-%02d/", localt->tm_year + 1900, localt->tm_mon + 1, localt->tm_mday);

	std::string fullpath = log_root_dir;
	fullpath.append(timebuf);
	log_create_dir(fullpath.c_str());

	if (log_create_file_time == curtime)
	{
		log_create_file_count++;
	}
	else
	{
		log_create_file_time = curtime;
		log_create_file_count = 0;
	}

	std::string tmp;
	std::string last_file;
	while (true)
	{
		tmp = fullpath;
		sprintf(timebuf, "%02d_%02d__%d.log", localt->tm_mon + 1, localt->tm_mday, log_create_file_count);
		tmp.append(timebuf);
		if(!log_file_is_exist(tmp.c_str()))
			break;
		log_create_file_count++;
		last_file = tmp;
	}
	if (last_file.empty())
	{
		fullpath = tmp;
	}
	else
	{
		log_create_file_count--;
		fullpath = last_file;
	}
	log_cur_day = localt->tm_mday;
	log_file_size = 0;

	return fullpath;
}

bool log_new_file()
{
	std::string log_path = log_new_file_name();
	return log_openfile(log_path.c_str());
}

bool log_init(const char* root_dir, int max_size)
{
	if (logFP)
		return false;

	if (root_dir != NULL && strlen(root_dir) > 0)
	{
		log_root_dir = root_dir;
		log_root_dir = log_string_replace_all(log_root_dir, "\\", "/");

		if (log_root_dir.back() != '/')
		{
			log_root_dir.append("/");
		}
	}

	if (max_size > 1024)
	{
		log_file_max_size = max_size;
	}

	return log_new_file();
}

void log_exit()
{
	if (logFP)
	{
		fclose(logFP);
		logFP = NULL;
	}
}

void log_update()
{
	log_queue_mutex.lock();
	if (log_write_queue.empty())
	{
		log_queue_mutex.unlock();
		return;
	}

	time_t tNow = time(NULL);
	tm t = *localtime(&tNow);
	char buf[256];
	snprintf(buf, 256, "[%d/%d %02d:%02d:%02d]\t", t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

	if (log_cur_day != t.tm_mday)
	{
		log_cur_day = t.tm_mday;
		log_new_file();
	}

	std::string str_log;
	bool issave = false;
	while (!log_write_queue.empty())
	{
		issave = log_write_queue.front().issave;

		if (!issave)
		{
			str_log = log_write_queue.front().content;
			printf(str_log.c_str());
		}
		else
		{
			str_log = buf;
			str_log.append(log_write_queue.front().content);
			//printf(str_log.c_str());
		}

		if (issave)
		{
			log_file_size = log_file_size + str_log.size();
			if (log_file_size >= log_file_max_size)
			{
				log_new_file();
			}
			if (logFP)
			{
				fwrite(str_log.c_str(), str_log.size(), 1, logFP);
			}
		}
		log_write_queue.pop();
	}
	log_queue_mutex.unlock();
}

void log(const char* format, ...)
{
#define LOG_MAX_STRING_LENGTH (1024*100)

	va_list ap;
	va_start(ap, format);

	char* buf = (char*)malloc(LOG_MAX_STRING_LENGTH);
	if (buf != nullptr)
	{
		vsnprintf(buf, LOG_MAX_STRING_LENGTH, format, ap);
		va_end(ap);

		log_queue_block block;
		block.issave = true;
		block.content = buf;
		block.content.append("\n");

		log_queue_mutex.lock();
		log_write_queue.push(block);
		log_queue_mutex.unlock();

		free(buf);
	}
	else
	{
		va_end(ap);
	}
}


void log_print(const char* format, ...)
{
#define LOG_MAX_STRING_LENGTH (1024*100)

	va_list ap;
	va_start(ap, format);

	char* buf = (char*)malloc(LOG_MAX_STRING_LENGTH);
	if (buf != nullptr)
	{
		vsnprintf(buf, LOG_MAX_STRING_LENGTH, format, ap);
		va_end(ap);

		log_queue_block block;
		block.issave = false;
		block.content = buf;
		block.content.append("\n");

		log_queue_mutex.lock();
		log_write_queue.push(block);
		log_queue_mutex.unlock();

		free(buf);
	}
	else
	{
		va_end(ap);
	}
}

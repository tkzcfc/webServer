#pragma once

#include "mongoose.h"
#include <string>
#include <vector>
#include "md5.h"

class http_msg
{
public:
	http_msg();
	http_msg(const http_message& msg);
	http_msg(http_message* msg);
	http_msg(const http_msg& other);

	http_msg& operator=(const http_msg& other);

	void change(const http_message* msg);
	const http_message* get_http_message_ptr() { return m_http_message; }

	std::string message;
	std::string body;
	std::string method;
	std::string uri;
	std::string proto;
	int resp_code;
	std::string resp_status_msg;
	std::string query_string;
	std::string header_names[MG_MAX_HTTP_HEADERS];
	std::string header_values[MG_MAX_HTTP_HEADERS];

protected:
	const http_message* m_http_message;
};

namespace tools
{

	void print_http_message(http_message* msg);

	std::string get_http_var(const std::string& buff, const std::string& name);

	std::string create_activ_code(int curIndex);

	uint32 getBufHash(const void *buf, uint32 len);

	std::string& string_replace_all(std::string& str, const std::string& old_value, const std::string& new_value);

	std::string inspect_dir_path(std::string& str);

	std::vector<std::string> get_cmd_params(const std::string& cmd);

	bool check_cmd_is_number(const std::string& cmd);

}
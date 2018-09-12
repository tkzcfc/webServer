#include "tools.h"


http_msg::http_msg()
{}

http_msg::http_msg(const http_message& msg)
{
	change(&msg);
}


http_msg::http_msg(http_message* msg)
{
	change(msg);
}

http_msg::http_msg(const http_msg& other)
{
	this->message = other.message;
	this->body = other.body;
	this->method = other.method;
	this->uri = other.uri;
	this->proto = other.proto;
	this->resp_code = other.resp_code;
	this->resp_status_msg = other.resp_status_msg;
	this->query_string = other.query_string;
	for (int i = 0; i < MG_MAX_HTTP_HEADERS; ++i)
	{
		this->header_names[i] = other.header_names[i];
		this->header_values[i] = other.header_values[i];
	}
}


http_msg& http_msg::operator=(const http_msg& other)
{
	this->message = other.message;
	this->body = other.body;
	this->method = other.method;
	this->uri = other.uri;
	this->proto = other.proto;
	this->resp_code = other.resp_code;
	this->resp_status_msg = other.resp_status_msg;
	this->query_string = other.query_string;
	for (int i = 0; i < MG_MAX_HTTP_HEADERS; ++i)
	{
		this->header_names[i] = other.header_names[i];
		this->header_values[i] = other.header_values[i];
	}
	return *this;
}

void http_msg::change(const http_message* msg)
{
	this->message = std::string(msg->message.p, msg->message.len);
	this->body = std::string(msg->body.p, msg->body.len);
	this->method = std::string(msg->method.p, msg->method.len);
	this->uri = std::string(msg->uri.p, msg->uri.len);
	this->proto = std::string(msg->proto.p, msg->proto.len);
	this->resp_code = msg->resp_code;
	this->query_string = std::string(msg->query_string.p, msg->query_string.len);
	this->resp_status_msg = std::string(msg->resp_status_msg.p, msg->resp_status_msg.len);

	for (int i = 0; i < MG_MAX_HTTP_HEADERS; ++i)
	{
		this->header_names[i] = std::string(msg->header_names[i].p, msg->header_names[i].len);
		this->header_values[i] = std::string(msg->header_values[i].p, msg->header_values[i].len);
	}
	m_http_message = msg;
}

namespace tools {

	std::string format_mg_str(const std::string& name, const mg_str& str)
	{
		std::string out = "[";
		out.append(name);
		out.append("]:\t");
		out.append(std::string(str.p, str.len));
		out.append("\n\n");
		return out;
	}

	void print_http_message(http_message* msg)
	{
		std::string str;

		str.append(format_mg_str("message", msg->message));
		str.append(format_mg_str("body", msg->body));
		str.append(format_mg_str("method", msg->method));
		str.append(format_mg_str("uri", msg->uri));
		str.append(format_mg_str("proto", msg->proto));
		str.append("[resp_code]:\t");
		str.append(std::to_string(msg->resp_code));
		str.append("\n");
		str.append(format_mg_str("resp_status_msg", msg->resp_status_msg));
		str.append(format_mg_str("query_string", msg->query_string));
		str.append(format_mg_str("message", msg->message));

		char szBuf[64];
		for (int i = 0; i < MG_MAX_HTTP_HEADERS; ++i)
		{
			if (msg->header_names[i].len > 0)
			{
				sprintf(szBuf, "header_names(%d)", i);
				str.append(format_mg_str(szBuf, msg->header_names[i]));
			}
			else
			{
				str.append("\n\n");
				break;
			}
		}
		for (int i = 0; i < MG_MAX_HTTP_HEADERS; ++i)
		{
			if (msg->header_values[i].len > 0)
			{
				sprintf(szBuf, "header_names(%d)", i);
				str.append(format_mg_str(szBuf, msg->header_values[i]));
			}
			else
			{
				str.append("\n\n");
				break;
			}
		}
		printf(str.c_str());
	}


	std::string get_http_var(const std::string& buff, const std::string& name)
	{
		static struct mg_str g_s_buff_str;
		const static int g_s_buff_len = 1024 * 1024;
		static char g_s_tmp_buff[g_s_buff_len];

		memset(g_s_tmp_buff, 0, g_s_buff_len);
		g_s_buff_str.p = buff.c_str();
		g_s_buff_str.len = buff.size();

		mg_get_http_var(&g_s_buff_str, name.c_str(), g_s_tmp_buff, g_s_buff_len);

		return g_s_tmp_buff;
	}
		
	std::string& string_replace_all(std::string& str, const std::string& old_value, const std::string& new_value)
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

	std::string inspect_dir_path(std::string& str)
	{
		str = string_replace_all(str, "\\", "/");
		if (str.empty())
			return str;
		if (str.back() != '/' && str.back() != '\\')
			str.append("/");
		return str;
	}
}
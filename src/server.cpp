#include"stdafx.h"
#include"hall.hpp"
inline std::string json_format(const std::string& json, const std::string& levelstr = "    ")
{
    auto getLevelStr = [str = levelstr](int level) {
        std::string levelStr;
        for (int i = 0; i < level; i++)
        {
            levelStr += str;
        }
        return levelStr;
        };
    std::string result;
    int level = 0;
    for (std::string::size_type index = 0; index < json.size(); index++)
    {
        char c = json[index];

        if (level > 0 && '\n' == json[json.size() - 1])
        {
            result += getLevelStr(level);
        }

        switch (c)
        {
        case '{':
        case '[':
            result = result + c + "\n";
            level++;
            result += getLevelStr(level);
            break;
        case ',':
            result = result + c + "\n";
            result += getLevelStr(level);
            break;
        case '}':
        case ']':
            result += "\n";
            level--;
            result += getLevelStr(level);
            result += c;
            break;
        case ':':
            result += c;
            result += " ";
            break;
        default:
            result += c;
            break;
        }
    }
    return result;
}
int main(int argc, char** argv) {
    srand(time(0));
	auto sink1 = std::make_shared<spdlog::sinks::wincolor_stderr_sink_mt>();
	auto sink2 = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Logs/log.txt", 1024 * 1024 * 5, 3);
	std::vector< spdlog::sink_ptr> sinks = { sink1,sink2 };
	auto logger = std::make_shared<spdlog::logger>("file_console", sinks.begin(), sinks.end());
	auto consolelogger = std::make_shared<spdlog::logger>("console", sink1);
	spdlog::set_default_logger(logger);

	std::shared_ptr<bw::server::hall> hall = std::make_shared<bw::server::hall>(5);
	std::vector<int> ports{ 22222 };
	/*if (argc == 1) {
		std::print("Usage:\nserver <port number> [<port number>...]\n");
		return 0;
	}
	else
	{
		for (int i = 1; i < argc; ++i) {
			ports.push_back(std::stoi(argv[i]));
		}
	}*/

    hall->start(ports);
	//using namespace bw;
	//using namespace std::literals;
	//server::basic_user ginfo;
	//ginfo.name = "张三"s;
	//ginfo.state = server::user_st::free;
	//ginfo.id = 3;
	//ginfo.authority = server::basic_user::ordinary;
	//std::string buf;
	//iguana::to_json(ginfo, buf);
 //   buf = transform_str(buf);
 //   std::println("info:{}", buf);
	//msg_t msg= wrap(control_msg{
	//	.type = control_msg::create,
	//	.content = buf,
	//	.id1 = ginfo.id,
	//	.target_type = control_msg::g
	//	}, msg_t::control);
 //   buf = "";
	//iguana::to_json(msg, buf);
 //   std::print("{}",json_format(buf));



 //   msg_t newmsg;
 //   iguana::from_json(newmsg, buf);
 //   //assert(newmsg == msg);
 //   control_msg newcon;
 //   iguana::from_json(newcon, newmsg.jsonstr);
 //   std::println("type:{}\ncontent:{}\nid1:{},target_type:{}",
 //       newcon.type, json_format(newcon.content), newcon.id1, newcon.target_type);

 //   server::basic_user gnewinfo;
 //   iguana::from_json(gnewinfo, newcon.content);
 //   std::println("name:{}\nstate:{}\nid:{}\nauthority:{}", gnewinfo.name, gnewinfo.state, gnewinfo.id, gnewinfo.authority);
	return 0;
}
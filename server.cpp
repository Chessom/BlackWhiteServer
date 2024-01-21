#include"stdafx.h"
#include"hall.h"
int main(int argc,char** argv) {

	auto sink1 = std::make_shared<spdlog::sinks::wincolor_stderr_sink_mt>();
	auto sink2 = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Logs/log.txt", 1024 * 1024 * 5, 3);
	std::vector< spdlog::sink_ptr> sinks = { sink1,sink2 };
	auto logger = std::make_shared<spdlog::logger>("LoggerName", sinks.begin(), sinks.end());
	spdlog::set_default_logger(logger);

	std::shared_ptr<bw::server::hall> hall = std::make_shared<bw::server::hall>(10);
	std::vector<int> ports;
	if (argc <= 1) {
		std::print("Usage:\nserver <port number> [<port number>...]\n");
		return 0;
	}
	else
	{
		for (int i = 1; i < argc; ++i) {
			ports.push_back(std::stoi(argv[i]));
		}
	}
	
	hall->start(ports);
	return 0;
}
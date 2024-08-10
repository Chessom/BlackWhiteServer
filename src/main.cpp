//#include<iostream>
//#include<print>
//#include<boost/asio.hpp>
//using namespace std;
//using namespace boost::asio;
//inline std::string json_format(const std::string& json, const std::string& levelstr = "    ")
//{
//    auto getLevelStr = [str = levelstr](int level) {
//        std::string levelStr;
//        for (int i = 0; i < level; i++)
//        {
//            levelStr += str;
//        }
//        return levelStr;
//        };
//    std::string result;
//    int level = 0;
//    for (std::string::size_type index = 0; index < json.size(); index++)
//    {
//        char c = json[index];
//
//        if (level > 0 && '\n' == json[json.size() - 1])
//        {
//            result += getLevelStr(level);
//        }
//
//        switch (c)
//        {
//        case '{':
//        case '[':
//            result = result + c + "\n";
//            level++;
//            result += getLevelStr(level);
//            break;
//        case ',':
//            result = result + c + "\n";
//            result += getLevelStr(level);
//            break;
//        case '}':
//        case ']':
//            result += "\n";
//            level--;
//            result += getLevelStr(level);
//            result += c;
//            break;
//        case ':':
//            result += c;
//            result += " ";
//            break;
//        default:
//            result += c;
//            break;
//        }
//    }
//    return result;
//}
//boost::asio::awaitable<void> co_read(ip::tcp::socket& socket) {
//    string jsonstr;
//    size_t size;
//    boost::system::error_code ec;
//    while (true)
//    {
//        try
//        {
//            auto temp = socket.available();
//            co_await async_read(socket, buffer(&size, sizeof(size)), use_awaitable);
//            temp = socket.available();
//            co_await async_read(socket, dynamic_buffer(jsonstr, size), use_awaitable);
//            std::println("msg:{}", jsonstr);
//        }
//        catch (const std::exception& ec)
//        {
//            std::print("read error:{}", ec.what());
//            co_return;
//        }
//    }
//}
//boost::asio::awaitable<void> co_write(ip::tcp::socket& socket) {
//    string jsonstr;
//    size_t size;
//    boost::system::error_code ec;
//    while (true)
//    {
//        print("Enter msg:");
//        cin >> jsonstr;
//        size = jsonstr.size();
//        try
//        {
//            co_await async_write(socket, buffer(&size, sizeof(size)), use_awaitable);
//            co_await async_write(socket, dynamic_buffer(jsonstr, size), use_awaitable);
//        }
//        catch (const std::exception& ec)
//        {
//            std::println("write error:{}", ec.what());
//            co_return;
//        }
//    }
//}
//int main() {
//	int port = 22222;
//	io_context io;
//    print("Enter mode(0:connect 1:accept):");
//    int mode = 0;
//    cin >> mode;
//	print("Enter the port:");
//	cin >> port;
//	auto ep = ip::tcp::endpoint(ip::tcp::v4(), port);
//	boost::system::error_code ec;
//    if (mode) {
//        ip::tcp::acceptor ac(io, ep);
//        auto socket = ac.accept(ec);
//        if (ec) {
//            print("{}", ec.what());
//        }
//        else {
//            print("Success!");
//        }
//        /*std::string json;
//        size_t size;
//        boost::system::error_code ec;
//        while (true) {
//            read(socket, buffer(&size, sizeof(size)), ec);
//            read(socket, dynamic_buffer(json, size), ec);
//            std::print("{}", json_format(json));
//        }*/
//        co_spawn(io, co_read(socket), detached);
//        co_spawn(io, co_write(socket), detached);
//        thread t([&io] {io.run(); });
//        t.detach();
//        this_thread::sleep_for(100000min);
//    }
//    else
//    {
//        ip::tcp::socket socket(io);
//        socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), port), ec);
//        if (ec) {
//            print("{}", ec.what());
//
//        }
//        else {
//            print("Success!");
//        }
//        /*std::string json;
//        size_t size;
//        boost::system::error_code ec;
//        while (true) {
//            read(socket, buffer(&size, sizeof(size)), ec);
//            read(socket, dynamic_buffer(json, size), ec);
//            if (ec) {
//                print("{}", ec.what());
//                return 0;
//            }
//            std::print("{}", json_format(json));
//        }*/
//        co_spawn(io, co_read(socket), detached);
//        co_spawn(io, co_write(socket), detached);
//        thread t([&io] {io.run(); });
//        t.detach();
//        this_thread::sleep_for(100000min);
//    }
//    
//	return 0;
//}

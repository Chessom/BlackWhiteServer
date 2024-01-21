#pragma once
#include"stdafx.h"
#include"room.h"
#include"gamer.h"
namespace bw::server {
	class hall :public room, public std::enable_shared_from_this<hall> {
	public:
		friend class gamer;
		hall(int default_room_n = 5)
			:rooms(default_room_n), room(0, "Default"),
			context_ptr_(std::make_shared<boost::asio::io_context>()){
			for (int i = 0; i < default_room_n; ++i) {
				rooms[i] = std::make_shared<room>(i + 1);
			}
		}
		void start(const std::vector<int>& ports) {
			for (auto port : ports)
				start(port);
		}
		void start(int portnum = 22222) {
			boost::asio::co_spawn(*context_ptr_, listener(
				boost::asio::ip::tcp::acceptor(*context_ptr_, { boost::asio::ip::tcp::v4(), (boost::asio::ip::port_type)portnum })
			), boost::asio::detached);
		}
		basic_gamer_ptr find_gamer(int gamer_id) {
			for (auto& g : gamers) {
				if (g->id == gamer_id) {
					return g;
				}
			}
		}
		int create_room(std::string room_name) {
			int newid = 0;
			if (!dissolved_room_id.empty()) {
				newid = dissolved_room_id.top();
				dissolved_room_id.pop();
				rooms[newid] = std::make_shared<room>(newid, room_name);
				return newid;
			}
			else {
				newid = rooms.size();
				rooms.push_back(std::make_shared<room>(newid, room_name));
				return newid;
			}
		}
		int del_room(int room_id) {
			if (room_id == 0) {
				spdlog::error("Cannot delete default room!");
				return -1;
			}
			if (rooms[room_id]) {
				rooms[room_id]->dissolve();
				rooms[room_id] = nullptr;
				dissolved_room_id.push(room_id);
				return 0;
			}
		}
		~hall() {
			context_ptr_->stop();
		}
	private:
		boost::asio::awaitable<void> listener(boost::asio::ip::tcp::acceptor acceptor) {
			for (;;){
				auto g = std::make_shared<gamer>(
					co_await acceptor.async_accept(boost::asio::use_awaitable),
					shared_from_this()
				);
				join(g);
				g->start();
			}
		}
		std::vector<room_ptr> rooms;
		std::priority_queue<int> dissolved_room_id;
		std::shared_ptr<boost::asio::io_context> context_ptr_;
	};
}
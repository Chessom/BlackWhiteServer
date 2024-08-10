#pragma once
#include"stdafx.h"
#include"room.h"
#include"message.hpp"
namespace bw::server {
	class user :public basic_user, public std::enable_shared_from_this<user> {
	public:
		using socket_t = boost::asio::ip::tcp::socket;
		user(socket_t socket, room_ptr room_p, int user_id);
		virtual void deliver(const message&);
		virtual void broadcast(const message&);
		virtual void start();
		virtual void join(room_ptr room_p);
		virtual void leave();
		virtual void stop();
		virtual void home();
		virtual void logout();
		virtual room_ptr current_room() { return room_; }
		bool in_hall() {
			if (room_ != nullptr) {
				return room_->is_default();
			}
			else{
				room_ = hall_;
				return true;
			}
		}
		virtual ~user() { spdlog::info("ID:{} Name:{} user left the server.", id, name); }
		const int sizelen = 5;
	private:
		virtual inline void handle_msg(const message&);
		boost::asio::awaitable<void> reader();
		boost::asio::awaitable<void> writer();
		std::deque<message> write_msg_queue;
		room_ptr room_ = nullptr, hall_ = nullptr;
		socket_t sock;
		boost::asio::steady_timer timer_;
	};
	using user_ptr = std::shared_ptr<user>;
}
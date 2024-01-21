#pragma once
#include"stdafx.h"
#include"room.h"
#include"message.hpp"
namespace bw::server {
	class gamer :public basic_gamer, std::enable_shared_from_this<gamer> {
	public:
		using socket_t = boost::asio::ip::tcp::socket;
		gamer(socket_t socket,room_ptr room_p);
		virtual void deliver(const message&);
		void broadcast(const message&);
		void start();
		void join(room_ptr room_p);
		void leave();
		void stop();
		void home();
	private:
		inline void handle_msg(const message&);
		boost::asio::awaitable<void> reader();
		boost::asio::awaitable<void> writer();
		std::deque<message> write_msg_queue;
		room_ptr room_ = nullptr, hall_ = nullptr;
		socket_t sock;
		boost::asio::steady_timer timer_;
	};
	using gamer_ptr = std::shared_ptr<gamer>;
}
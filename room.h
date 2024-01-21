#pragma once
#include"stdafx.h"

#include"basic_gamer.hpp"
namespace bw::server {
	class room_info {
	public:
		room_info() = default;
		room_info(int room_id, int room_owner, std::string room_name, int room_state) :id(room_id), owner(room_owner), name(room_name), state(room_state) {};
		int id = -1, owner = 0;
		std::string name = "Default";
		int state = none;
		enum { gaming, end, prepared, none };
		virtual ~room_info() = default;
	};
	REFLECTION(room_info, id, owner, name, state);
	class room :public room_info {
	public:
		room() = default;
		room(int room_id, std::string room_name = "Default") :room_info{ room_id, 0, room_name, none } {};
		void deliver(const message& msg);
		bool is_default() const {
			return id == 0;
		}
		void join(basic_gamer_ptr);
		void leave(basic_gamer_ptr);
		void dissolve();
		int clear_msg(int type, int num);
		void load_info(const room_info&);
		std::set<basic_gamer_ptr> gamers;
		virtual ~room() = default;
	private:
		std::deque<message> msg_queue;
		enum { max_msg_num = 500 };
	};

	using room_ptr = std::shared_ptr<room>;
}
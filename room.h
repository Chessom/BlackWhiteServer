#pragma once
#include"basic_user.hpp"
#include"room_info.h"
#include<set>
#include<boost/container/flat_map.hpp>
namespace bw::server {
	class room :public room_info {
	public:
		room() = default;
		room(int room_id, std::string room_name = "Default") :room_info{ room_id, 0, room_name, none } {};
		virtual void deliver(const message& msg);
		bool is_default() const {
			return id == 0;
		}
		void join(basic_user_ptr);
		virtual void leave(basic_user_ptr);
		void dissolve();
		int clear_msg(int type, int num);
		void load_info(const room_info&);
		std::set<basic_user_ptr> users;
		virtual void handle_msg(basic_user_ptr, const msg_t&);
		virtual ~room() = default;
	protected:
		std::deque<message> msg_queue;
		boost::container::flat_map<std::string, std::list<basic_user_ptr>> user_matching;
		boost::container::flat_map<int, msg_t> prepare_msgs;
		//std::unordered_map<game::game_setting, int> user_matching;
		enum { max_msg_num = 500 };
	};

	using room_ptr = std::shared_ptr<room>;
}
#pragma once
#include"basic_user.hpp"
namespace bw::server {
	class room_info {
	public:
		room_info() = default;
		room_info(int room_id, int room_owner = 0, std::string room_name = "Default", int room_state = none) :id(room_id), owner(room_owner), name(room_name), state(room_state) {};
		int id = -1, owner = 0;
		std::string name = "Default";
		int state = none;
		int usersize = 0;
		int prepared_users = 0;
		enum { gaming, end, prepared, none };
		virtual ~room_info() = default;
	};
	enum { hall_id = 0 };
	REFLECTION(room_info, id, owner, name, state, usersize);
}
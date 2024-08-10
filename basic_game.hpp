#pragma once
#include"basic_user.hpp"
#include"room_info.h"
namespace bw::server::games {
	struct basic_game {
		virtual void handle_game_msg(basic_user_ptr gptr, const msg_t& msg, room_info& info) = 0;
		virtual ~basic_game() = default;
	};
}
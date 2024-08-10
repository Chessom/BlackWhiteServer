#pragma once
#include"basic_game.hpp"
#include<boost/container/flat_map.hpp>
namespace bw::server::games {
	struct othello :basic_game {
		using brd_size_user = std::pair<int, basic_user_ptr>;

		virtual void handle_game_msg(basic_user_ptr gptr, const msg_t& msg, room_info& info) override {
			game_msg gmsg;
			struct_json::from_json(gmsg, msg.jsonstr);
			int brd_size;
			switch (gmsg.type) {
			case game_msg::prepare:
				if (info.state == room_info::end || info.state == room_info::none) {
					brd_size = std::stoi(gmsg.board);
					if (users.find(brd_size) != users.end()) {
						auto& glist = users[brd_size];
						if (glist.empty()) {
							glist.push_back(gptr);
						}
						else {
							auto gptr2 = glist.front();
							glist.pop_front();
						}
					}
					else {

					}
				}
				break;
			case game_msg::start:
				if (info.state == room_info::prepared) {
					
					info.state = room_info::gaming;
				}
				break;
			case game_msg::move:

				break;
			case game_msg::end:

				break;
			default:
				std::unreachable();
			}
		}
		boost::container::flat_map<std::string, std::list<basic_user_ptr>> users;
	};
}
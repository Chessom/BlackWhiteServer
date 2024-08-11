#include "room.h"
#include "gamer.hpp"

void bw::server::room::deliver(const message& msg)
{
	msg_queue.push_back(msg);
	if (msg_queue.size() >= max_msg_num) {
		clear_msg(message::str, max_msg_num / 2);
	}
	for (auto& g : users) {
		g->deliver(msg);
	}
}

void bw::server::room::join(basic_user_ptr p) {
	if (owner == 0) {
		owner = p->id;
	}
	for (auto& msg : msg_queue) {
		if (msg.type == msg_t::str || msg.type == msg_t::notice)
			p->deliver(msg);
	}
	users.insert(p);
	usersize = users.size();
}

void bw::server::room::leave(basic_user_ptr p) {
	if (owner == p->id)
		owner = 0;
	users.erase(p);
	usersize = users.size();
	if (p->state == user_st::prepared) {
		game_msg gmmsg = prepare_msgs[p->id];
		auto& mlist = user_matching[gmmsg.board];
		for (auto iter = mlist.begin(); iter != mlist.end(); ++iter) {
			if (*iter == p) {
				mlist.erase(iter);
				break;
			}
		}
	}
	else if (p->state == user_st::gaming) {
		gaming_users.erase(p);
		end_game(game_msg::gamer_quit_or_disconnect);
	}
	if (id != 0 && usersize == 0) {
		msg_queue.clear();
	}
}

void bw::server::room::dissolve() {
	owner = 0;
	for (auto& g : users) {
		g->deliver(wrap(control_msg{ .type = control_msg::leave,.content = "Room has been dissolved" ,.id1 = g->id,.id2 = id }, msg_t::control));
		g->home();
		leave(g);
	}
	usersize = users.size();
}

int bw::server::room::clear_msg(int type, int num) {
	int cnt = 0;
	for (auto it = msg_queue.begin(); it != msg_queue.end(); ++it) {
		if (it->type == type) {
			msg_queue.erase(it);
			++cnt;
			if (cnt >= num) {
				break;
			}
		}
	}
	return cnt;
}

void bw::server::room::load_info(const room_info& info)
{
	id = info.id;
	owner = info.owner;
	name = info.name;
	state = info.state;
}

void bw::server::room::handle_msg(basic_user_ptr gptr, const msg_t& msg){
	if (msg.type == msg_t::game) {
		game_msg gmsg;
		struct_json::from_json(gmsg, msg.jsonstr);
		switch (gmsg.type) {
		case game_msg::prepare://暂时先实现只有两个人的联机
			prepare_msgs[gptr->id] = gmsg;
			if (state == end || state == none) {
				auto& user_list = user_matching[gmsg.board];
				if ( user_list.size() != 0 && user_list.front()->id != gptr->id) {//组成两人联机
					basic_user_ptr op = user_list.front();
					game_msg op_msg = prepare_msgs[user_list.front()->id];

					core::color c = rand() % 2;

					basic_gamer_info info;
					struct_json::from_json(info, gmsg.movestr);
					info.col = c;
					gmsg.movestr = "";
					struct_json::to_json(info, gmsg.movestr);
					msg_t new_msg = wrap(gmsg, msg_t::game);

					basic_gamer_info op_info;
					struct_json::from_json(op_info, op_msg.movestr);
					op_info.col = core::op_col(c);
					op_msg.movestr = "";
					struct_json::to_json(op_info, op_msg.movestr);
					msg_t new_opmsg = wrap(op_msg, msg_t::game);

					op->deliver(new_msg);
					op->deliver(new_opmsg);
					gptr->deliver(new_msg);
					gptr->deliver(new_opmsg);

					auto start_msg = wrap(game_msg{
						.type = game_msg::start,
						.board = gmsg.board,
					}, msg_t::game);
					op->deliver(start_msg);
					gptr->deliver(start_msg);
					user_list.pop_front();

					gaming_users.insert({ op,gptr });
					op->state = user_st::gaming;
					gptr->state = user_st::gaming;
					state = gaming;
					spdlog::info("gamer {} and {} start a(an) {} game", gptr->name, op->name, gmsg.board);
				}
				else {
					user_list.push_back(gptr);
					prepare_msgs[gptr->id] = gmsg;
					gptr->state = user_st::prepared;

				}
			}
			break;
		case game_msg::start:
			if (state == prepared) {
				prepared_users = 0;
				state = gaming;
			}
			break;
		case game_msg::move:
			for (auto& gp : gaming_users) {
				if (gp->id != gptr->id) {
					gp->deliver(msg);
				}
			}
			break;
		case game_msg::end:
			state = end;
			for (auto& gp : gaming_users) {
				gp->deliver(msg);
				gp->state = user_st::free;
			}
			gaming_users.clear();
			break;
		default:
			std::unreachable();
		}
	}
}

void bw::server::room::end_game(int code) {
	if (state != gaming) {
		return;
	}
	msg_t end_msg = wrap(
		game_msg{
			.type = game_msg::end,
			.id = code,
		},
		msg_t::game
	);
	for (auto& gp : gaming_users) {
		gp->deliver(end_msg);
		gp->state = user_st::free;
	}
	gaming_users.clear();
	state = end;
}

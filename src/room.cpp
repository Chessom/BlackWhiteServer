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

void bw::server::room::deliver_gamers(const message& msg) {
	game_msg_queue.push_back(msg);
	for (auto& g : gaming_users) {
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
	else if (p->state == user_st::watching) {
		gaming_users.erase(p);
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

static bool is_two_gamer_game(std::string board_str) {
	for (int i = 0; i < board_str.size(); ++i) {
		if (board_str[i] == ' ') {
			auto game_type = board_str.substr(0, i);
			return game_type == "othello" || game_type == "tictactoe" || game_type == "ataxx" || game_type == "gobang";
		}
	}
}

void bw::server::room::handle_msg(basic_user_ptr gptr, const msg_t& msg){
	if (msg.type == msg_t::game) {
		game_msg gmsg;
		struct_json::from_json(gmsg, msg.jsonstr);
		switch (gmsg.type) {
		case game_msg::prepare://暂时先实现只有两个人的联机
			prepare_msgs[gptr->id] = gmsg;
			if (state == end || state == none) {
				auto& user_set = user_matching[gmsg.board];
				user_set.insert(gptr);
				prepare_msgs[gptr->id] = gmsg;

				if (user_set.size() == 2) {//组成两人联机
					two_gamer_init(user_set);
				}
				else {
					gptr->state = user_st::prepared;
				}
			}
			break;
		case game_msg::watch:
			gaming_users.insert(gptr);
			gptr->state = user_st::watching;
			gptr->deliver(msg);
			for (auto& pre_msg : game_msg_queue) {
				gptr->deliver(pre_msg);
			}
			break;
		case game_msg::start://暂时无用
			if (state == prepared) {
				state = gaming;
			}
			break;
		case game_msg::move:
			deliver_gamers(msg);
			break;
		case game_msg::end:
			state = end;
			for (auto& gp : gaming_users) {
				gp->deliver(msg);
				gp->state = user_st::free;
			}
			gaming_users.clear();
			game_msg_queue.clear();
			break;
		default:
			std::unreachable();
		}
	}
}

void bw::server::room::two_gamer_init(std::set<basic_user_ptr>& user_set)
{
	auto iter = user_set.begin();
	basic_user_ptr u0 = *iter;
	++iter;
	basic_user_ptr u1 = *iter;

	gaming_users.insert_range(user_set);

	game_msg msg0 = prepare_msgs[u0->id];
	game_msg msg1 = prepare_msgs[u1->id];

	core::color c = rand() % 2;

	basic_gamer_info info1;
	struct_json::from_json(info1, msg1.movestr);
	info1.col = c;
	msg1.movestr = "";
	struct_json::to_json(info1, msg1.movestr);
	msg_t new_msg1 = wrap(msg1, msg_t::game);

	basic_gamer_info info0;
	struct_json::from_json(info0, msg0.movestr);
	info0.col = core::op_col(c);
	msg0.movestr = "";
	struct_json::to_json(info0, msg0.movestr);
	msg_t new_msg0 = wrap(msg0, msg_t::game);

	deliver_gamers(new_msg1);
	deliver_gamers(new_msg0);

	auto start_msg = wrap(game_msg{
		.type = game_msg::start,
		.board = msg1.board,
		}, msg_t::game);
	deliver_gamers(start_msg);
	user_set.clear();

	u0->state = user_st::gaming;
	u1->state = user_st::gaming;
	state = gaming;
	spdlog::info("gamer {} and {} start a(an) {} game", u1->name, u0->name, msg1.board);
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
	game_msg_queue.clear();
	state = end;
}

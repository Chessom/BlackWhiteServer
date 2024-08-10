#include "room.h"

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
			prepare_msgs[gptr->id] = msg;
			if (state == end || state == none) {
				auto& user_list = user_matching[gmsg.board];
				if (user_list.size() != 0) {
					basic_user_ptr op = user_list.front();
					msg_t op_msg = prepare_msgs[user_list.front()->id];
					op->deliver(msg);
					op->deliver(op_msg);
					gptr->deliver(msg);
					gptr->deliver(op_msg);
					user_list.pop_front();

				}
				else {
					user_list.push_back(gptr);
					prepare_msgs[gptr->id] = msg;
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

			break;
		case game_msg::end:

			break;
		default:
			std::unreachable();
		}
	}
}

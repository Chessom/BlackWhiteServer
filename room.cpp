﻿#include "room.h"

void bw::server::room::deliver(const message& msg)
{
	msg_queue.push_back(msg);
	if (msg_queue.size() >= max_msg_num) {
		clear_msg(message::str, 20);
	}
	for (auto& g : gamers) {
		g->deliver(msg);
	}
}

void bw::server::room::join(basic_gamer_ptr p) {
	if (owner == 0) {
		owner = p->id;
	}
	for (auto& msg : msg_queue) {
		if (msg.type == msg_t::str)
			p->deliver(msg);
	}
	gamers.insert(p);
	gamersize = gamers.size();
}

void bw::server::room::leave(basic_gamer_ptr p) {
	if (owner == p->id)
		owner = 0;
	gamers.erase(p);
	gamersize = gamers.size();
	if (gamersize == 0) {
		msg_queue.clear();
	}
}

void bw::server::room::dissolve() {
	owner = 0;
	for (auto& g : gamers) {
		g->deliver(wrap(control_msg{ .type = control_msg::leave,.content = "Room has been dissolved" ,.id1 = g->id,.id2 = id }, msg_t::control));
		g->home();
		leave(g);
	}
	gamersize = gamers.size();
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

void bw::server::room::handle_msg(basic_gamer_ptr, const msg_t&)
{
}

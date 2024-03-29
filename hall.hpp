#pragma once
#include"gamer.h"
namespace bw::server {
	class hall :public room, public std::enable_shared_from_this<hall> {
	public:
		friend class gamer;
		hall(int default_room_n = 10)
			:rooms(default_room_n + 1), room(0, "Hall"),
			context_ptr_(std::make_shared<boost::asio::io_context>()){
			for (int i = 1; i <= default_room_n; ++i) {
				rooms[i] = std::make_shared<room>(i);
				dissolved_room_id.push(i);
			}
		}
		void start(const std::vector<int>& ports){
			rooms[0] = shared_from_this();
			deliver(wrap(str_msg{ .content = "Welcome to this default server. This server belongs to the author. Hope you have fun!",
				.target_type = str_msg::g,
				.id1 = 0,
				.id2 = -1
				}, msg_t::str));
			for (auto port : ports)
				boost::asio::co_spawn(*context_ptr_, listener(
					boost::asio::ip::tcp::acceptor(*context_ptr_, { boost::asio::ip::tcp::v4(), (boost::asio::ip::port_type)port })
				), boost::asio::detached);
			context_ptr_->run();
		}
		basic_gamer_ptr find_gamer(int gamer_id) {
			for (auto& g : gamers) {
				if (g->id == gamer_id) {
					return g;
				}
			}
			return nullptr;
		}
		int create_room(std::string room_name) {
			int newid = -1;
			if (!dissolved_room_id.empty()) {
				newid = dissolved_room_id.top();
				dissolved_room_id.pop();
				rooms[newid] = std::make_shared<room>(newid, room_name);
				return newid;
			}
			else {
				newid = rooms.size();
				rooms.push_back(std::make_shared<room>(newid, room_name));
				return newid;
			}
		}
		int distribute_gamer_id() {
			int newid = -1;
			if (!gamer_id_left.empty()) {
				newid = gamer_id_left.top();
				gamer_id_left.pop();
			}
			else {
				newid = gamers.size() + 1;
			}
			return newid;
		}
		int del_room(int room_id) {
			if (room_id == 0) {
				spdlog::error("Cannot delete default room!");
				return -1;
			}
			if (rooms[room_id]) {
				rooms[room_id]->dissolve();
				rooms[room_id] = nullptr;
				dissolved_room_id.push(room_id);
				return 0;
			}
		}
		virtual void handle_msg(basic_gamer_ptr basic_sender_ptr, const msg_t& msg) override{
			auto sender = std::dynamic_pointer_cast<gamer>(basic_sender_ptr);
			int type = msg.type;
			if (type == msg_t::invalid)
				throw std::runtime_error("Invalid Control Message");
			if (type == msg_t::control) {
				control_msg con_m;
				struct_json::from_json(con_m, msg.jsonstr);
				if (con_m.type == control_msg::create) {
					msg_t ret;
					if (con_m.target_type == control_msg::r) {
						room_info info;
						int newid = create_room(con_m.content);
						struct_json::from_json(info, con_m.content);
						rooms[newid]->load_info(info);
						ret = wrap(ret_msg{ .value = success ,.ret_str = std::to_string(newid) }, msg_t::ret);
					}
					else if(con_m.target_type == control_msg::g){
						basic_gamer info;
						struct_json::from_json(info, con_m.content);
						auto newid = sender->id;
						sender->load_info(info);
						sender->id = newid;
						std::string newinfo;
						iguana::to_json(static_cast<basic_gamer>(*sender), newinfo);
						ret = wrap(control_msg{
							.type = control_msg::create,
							.content = newinfo,
							.id1 = newid,
							.target_type = control_msg::g
							}, msg_t::control);
						spdlog::info("ID:{} Name:{} gamer login", newid, sender->name);
					}
					else {
						ret = wrap(ret_msg{ .value = failed,.ret_str = "Invalid target type" }, msg_t::ret);
					}
					sender->deliver(ret);
				}
				else if (con_m.type == control_msg::update) {//修改room等的info id1为gamer id,id2为room id;
					msg_t ret;
					if (con_m.target_type == control_msg::g) {
						basic_gamer ginfo;
						if (con_m.id1 == sender->id) {
							struct_json::from_json(ginfo, con_m.content);
							sender->load_info(ginfo);
							ret = wrap(ret_msg{ .value = success }, msg_t::ret);
							sender->deliver(msg);
						}
						else if (auto g = find_gamer(con_m.id1); g->authority == gamer::admin) {
							struct_json::from_json(ginfo, con_m.content);
							g->load_info(ginfo);
							ret = wrap(ret_msg{ .value = success }, msg_t::ret);
							g->deliver(msg);
						}
						else {
							ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
						}
					}
					else if (con_m.target_type == control_msg::r) {
						room_info rinfo;
						if (rooms[con_m.id2]->owner == con_m.id1 || find_gamer(con_m.id1)->authority == gamer::admin) {
							struct_json::from_json(rinfo, con_m.content);
							rooms[con_m.id2]->load_info(rinfo);
							ret = wrap(ret_msg{ .value = success }, msg_t::ret);
						}
						else
							ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
					}
					else {
						ret = wrap(ret_msg{ .value = failed,.ret_str = "Fatal:Data structure damaged" }, msg_t::ret);
						spdlog::error("Data structure damaged;id:{}", sender->id);
					}
					sender->deliver(ret);
				}
				else if (con_m.type == control_msg::del) {
					msg_t ret;
					if (con_m.target_type == control_msg::r) {
						if (find_gamer(con_m.id1)->authority == gamer::admin) {
							auto& temp_room_p = rooms[con_m.id2];
							if (temp_room_p != nullptr) {
								temp_room_p->dissolve();
								temp_room_p = nullptr;
								ret = msg;
							}
							else
							{
								ret = wrap(ret_msg{ .value = failed,.ret_str = "Room not exist" }, msg_t::ret);
							}
						}
						else {
							if (rooms[con_m.id2] != nullptr) {
								if (rooms[con_m.id2]->owner == con_m.id1) {
									rooms[con_m.id2]->dissolve();
									rooms[con_m.id2] = nullptr;
									ret = msg;
								}
								else {
									ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
								}
							}
							else {
								ret = wrap(ret_msg{ .value = failed,.ret_str = "Room not exist" }, msg_t::ret);
							}
						}
					}
					else if (con_m.target_type == control_msg::g) {
						gamer_ptr temp_p = nullptr;
						if (con_m.id1 == sender->id) {
							ret = wrap(ret_msg{ .value = success }, msg_t::ret);
							sender->deliver(ret);
							if (sender->in_hall()) {
								sender->leave();
							}
							else {
								sender->leave();
								sender->leave();
							}
						}
						else if (auto g = find_gamer(con_m.id1); g->authority == gamer::admin) {
							temp_p = std::dynamic_pointer_cast<gamer>(g);
							temp_p->deliver(wrap(str_msg{ .content = "You have been kicked out of server!",.id1 = sender->id }, msg_t::str));
							if (temp_p->in_hall()) {
								temp_p->leave();
							}
							else {
								temp_p->leave();
								temp_p->leave();
							}
						}
						else {
							ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
						}
					}
					else {
						ret = wrap(ret_msg{ .value = failed,.ret_str = "Invalid target type" }, msg_t::ret);
					}
				}
				else if (con_m.type == control_msg::join) {
					auto gp = std::dynamic_pointer_cast<gamer>(find_gamer(con_m.id1));
					gp->join(rooms[con_m.id2]);
					sender->deliver(msg);
				}
				else if (con_m.type == control_msg::leave) {
					auto gp = std::dynamic_pointer_cast<gamer>(find_gamer(con_m.id1));
					gp->leave();
					sender->deliver(msg);
				}
				else if (con_m.type == control_msg::none) {
					sender->deliver(wrap(ret_msg{ .value = success,.ret_str = "Always success" }, msg_t::ret));
				}
				else {
					sender->deliver(wrap(ret_msg{ .value = failed,.ret_str = "FATAL:Data structure damaged" }, msg_t::ret));
					spdlog::error("Data structure damaged;id:{}", con_m.id1);
				}
			}
			else if (type == msg_t::str) {
				str_msg strmsg;
				struct_json::from_json(strmsg, msg.jsonstr);
				auto gp = std::dynamic_pointer_cast<gamer>(find_gamer(strmsg.id1));
				if (strmsg.target_type == str_msg::g) {
					gp->deliver(wrap(str_msg{ .content = strmsg.content,.id1 = sender->id }, msg_t::str));
				}
				else {
					if (gp->in_hall()) {
						if (gp->authority == gamer::admin) {
							gp->broadcast(wrap(strmsg, msg_t::str));
						}
						else {
							gp->deliver(wrap(ret_msg{ .value = failed,.ret_str = "Only administrators can broadcast messages in whole server." }, msg_t::ret));
						}
					}
					else {
						gp->broadcast(msg);
					}
				}
			}
			else if (type == msg_t::game) {
				game_msg gmsg;
				struct_json::from_json(gmsg, msg.jsonstr);
				if (sender->in_hall()) {
					auto st = sender->current_room()->state;
					if (st == room_info::prepared || st == room_info::gaming) {
						sender->broadcast(wrap(gmsg, msg_t::game));
					}
				}
			}
			else if (type == msg_t::get) {
				get_msg gmsg;
				iguana::from_json(gmsg, msg.jsonstr);
				ret_msg ret;
				if (gmsg.get_type == "room_info") {
					std::vector<room_info> infos;
					infos.push_back(static_cast<room_info>(*sender->current_room()));
					if (gmsg.ids.empty()) {
						for (int i = 0; i < rooms.size(); ++i) {
							auto& r = rooms[i];
							infos.push_back(static_cast<room_info>(*r));
						}
					}
					else {
						for (auto& rid : gmsg.ids) {
							infos.push_back(static_cast<room_info>(*rooms[rid]));
						}
					}
					struct_json::to_json(infos, ret.ret_str);
					ret.value = success;
					ret.ret_type = "room_info";
				}
				else if (gmsg.get_type == "gamer_info") {
					std::vector<basic_gamer> infos;
					for (auto& gid : gmsg.ids) {
						infos.push_back(*find_gamer(gid));
					}
					iguana::to_json(infos, ret.ret_str);
					ret.value = success;
					ret.ret_type = "gamer_info";
				}
				else if (gmsg.get_type == "notices") {

				}
				else {

				}
				sender->deliver(wrap(ret, msg_t::ret));
			}
			else {
				sender->deliver(wrap(ret_msg{ .value = failed,.ret_str = "FATAL:Data structure damaged" }, msg_t::ret));
				spdlog::error(msg.jsonstr);
			}
		}
		virtual void leave(basic_gamer_ptr p) override {
			gamer_id_left.push(p->id);
			room::leave(p);
		}
		virtual ~hall() = default;
	private:
		boost::asio::awaitable<void> listener(boost::asio::ip::tcp::acceptor acceptor) {
			for (;;){
				gamer_ptr g = std::make_shared<gamer>(
					co_await acceptor.async_accept(boost::asio::use_awaitable),
					shared_from_this(),
					-1
				);
				g->id = distribute_gamer_id();
				g->start();
			}
		}
		std::vector<room_ptr> rooms;
		std::priority_queue<int> dissolved_room_id, gamer_id_left;
		std::shared_ptr<boost::asio::io_context> context_ptr_;
	};
}
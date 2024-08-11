#pragma once
#include"user.h"
namespace bw::server {
	class hall :public room, public std::enable_shared_from_this<hall> {
	public:
		friend class user;
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
			notice("Welcome to this server. This server belongs to the author of the program. Hope you have fun!");
			for (auto port : ports)
				boost::asio::co_spawn(*context_ptr_, listener(
					boost::asio::ip::tcp::acceptor(*context_ptr_, { boost::asio::ip::tcp::v4(), (boost::asio::ip::port_type)port })
				), boost::asio::detached);
			context_ptr_->run();
		}
		basic_user_ptr find_user(int user_id) {
			for (auto& g : users) {
				if (g->id == user_id) {
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
		int distribute_user_id() {
			int newid = -1;
			if (!user_id_left.empty()) {
				newid = user_id_left.top();
				user_id_left.pop();
			}
			else {
				newid = users.size() + 1;
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
		virtual void handle_msg(basic_user_ptr basic_sender_ptr, const msg_t& msg) override{
			auto sender = std::dynamic_pointer_cast<user>(basic_sender_ptr);
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
						basic_user info;
						struct_json::from_json(info, con_m.content);
						auto newid = sender->id;
						sender->load_info(info);
						sender->id = newid;
						std::string newinfo;
						iguana::to_json(static_cast<basic_user>(*sender), newinfo);
						ret = wrap(control_msg{
							.type = control_msg::create,
							.content = newinfo,
							.id1 = newid,
							.target_type = control_msg::g
							}, msg_t::control);
						spdlog::info("ID:{} Name:{} user login", newid, sender->name);
					}
					else {
						ret = wrap(ret_msg{ .value = failed,.ret_str = "Invalid target type" }, msg_t::ret);
					}
					sender->deliver(ret);
				}
				else if (con_m.type == control_msg::update) {//修改room等的info id1为user id,id2为room id;
					msg_t ret;
					if (con_m.target_type == control_msg::g) {
						basic_user ginfo;
						if (con_m.id1 == sender->id) {
							struct_json::from_json(ginfo, con_m.content);
							sender->load_info(ginfo);
							ret = wrap(ret_msg{ .value = success }, msg_t::ret);
							sender->deliver(msg);
						}
						else if (auto g = find_user(con_m.id1); g->authority == user::admin) {
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
						if (!(con_m.id2 > 0 && con_m.id2 < rooms.size())) {
							ret = wrap(ret_msg{ .value = failed,.ret_str = "Room not found" }, msg_t::ret);
						}
						else if (rooms[con_m.id2]->owner == con_m.id1 || find_user(con_m.id1)->authority == user::admin) {
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
						if (con_m.id2 > 0 && con_m.id2 < rooms.size() && find_user(con_m.id1)->authority == user::admin) {
							auto& temp_room_p = rooms[con_m.id2];
							if (temp_room_p != nullptr) {
								temp_room_p->dissolve();
								temp_room_p = nullptr;
								ret = msg;
							}
							else
							{
								ret = wrap(ret_msg{ .value = failed,.ret_type = "operation_failed", .ret_str = "Room not exist" }, msg_t::ret);
							}
						}
						else {
							if (con_m.id2 > 0 && con_m.id2 < rooms.size() && rooms[con_m.id2] != nullptr) {
								if (rooms[con_m.id2]->owner == con_m.id1) {
									rooms[con_m.id2]->dissolve();
									rooms[con_m.id2] = nullptr;
									ret = msg;
								}
								else {
									ret = wrap(ret_msg{ .value = failed,.ret_type = "operation_failed",.ret_str = "No permission" }, msg_t::ret);
								}
							}
							else {
								ret = wrap(ret_msg{ .value = failed,.ret_type = "operation_failed",.ret_str = "Room not exist" }, msg_t::ret);
							}
						}
					}
					else if (con_m.target_type == control_msg::g) {
						user_ptr temp_p = nullptr;
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
						else if (auto g = find_user(con_m.id1); g->authority == user::admin) {
							temp_p = std::dynamic_pointer_cast<user>(g);
							temp_p->deliver(wrap(str_msg{ .content = "You have been kicked out of server!",.id1 = sender->id,.name = sender->name }, msg_t::str));
							if (temp_p->in_hall()) {
								temp_p->leave();
							}
							else {
								temp_p->leave();
								temp_p->leave();
							}
						}
						else {
							ret = wrap(ret_msg{ .value = failed,.ret_type = "operation_failed",.ret_str = "No permission" }, msg_t::ret);
						}
					}
					else {
						ret = wrap(ret_msg{ .value = failed,.ret_type = "operation_failed",.ret_str = "Invalid target type" }, msg_t::ret);
					}
				}
				else if (con_m.type == control_msg::join) {
					auto gp = std::dynamic_pointer_cast<user>(find_user(con_m.id1));
					if (!(con_m.id2 > 0 && con_m.id2 < rooms.size())) {
						sender->deliver(wrap(
							ret_msg{
								.value = failed,
								.ret_type = "join_room_failed",
								.ret_str = "Room not found"
							},
							msg_t::ret
						));	
					}else if(!gp->in_hall()){
						sender->deliver(wrap(
							ret_msg{
								.value = failed,
								.ret_type = "join_room_failed",
								.ret_str = "Already in a room"
							},
							msg_t::ret
						));
					}
					else {
						gp->join(rooms[con_m.id2]);
						con_m.content = "";
						struct_json::to_json(static_cast<room_info>(*rooms[con_m.id2]), con_m.content);
						sender->deliver(wrap(con_m, msg_t::control));
					}
				}
				else if (con_m.type == control_msg::leave) {
					auto gp = std::dynamic_pointer_cast<user>(find_user(con_m.id1));
					gp->leave();
					sender->deliver(msg);
				}
				else if (con_m.type == control_msg::none) {
					sender->deliver(wrap(ret_msg{ .value = success,.ret_str = "Always success" }, msg_t::ret));
				}
				else {
					sender->deliver(wrap(ret_msg{ .value = failed,.ret_str = "FATAL:Data structure damaged" }, msg_t::ret));
					spdlog::error("Data structure damaged; id:{} msg:{}", con_m.id1, msg.jsonstr);
				}
			}
			else if (type == msg_t::str) {
				str_msg strmsg;
				struct_json::from_json(strmsg, msg.jsonstr);
				auto gp = std::dynamic_pointer_cast<user>(find_user(strmsg.id2));
				if (strmsg.target_type == str_msg::g) {
					gp->deliver(msg);
				}
				else {
					if (gp->in_hall()) {
						if (gp->authority == user::admin) {
							gp->broadcast(msg);
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
				if (!sender->in_hall()) {
					sender->current_room()->handle_msg(sender, msg);
				}
				/*game_msg gmsg;
				struct_json::from_json(gmsg, msg.jsonstr);
				if (!sender->in_hall()) {
					auto st = sender->current_room()->state;
					if (st == room_info::prepared || st == room_info::gaming) {
						sender->broadcast(wrap(gmsg, msg_t::game));
					}
				}*/
			}
			else if (type == msg_t::get) {
				get_msg gmsg;
				struct_json::from_json(gmsg, msg.jsonstr);
				ret_msg ret;
				if (gmsg.get_type == "current_room_info") {
					std::vector<room_info> infos;
					if (!sender->in_hall()) {
						infos.push_back(static_cast<room_info>(*sender->current_room()));
						struct_json::to_json(infos, ret.ret_str);
						ret.value = success;
						ret.ret_type = "current_room_info";
					}
					else {
						ret.value = failed;
						ret.ret_type = "current_room_info";
						ret.ret_str = "Not in any room";
					}
				}
				else if (gmsg.get_type == "search_room_info") {
					std::vector<room_info> infos;
					int search_id = gmsg.ids.front();
					if (search_id > 0 && search_id < rooms.size()) {
						infos.push_back(static_cast<room_info>(*rooms.at(search_id)));
						struct_json::to_json(infos, ret.ret_str);
						ret.value = success;
						ret.ret_type = "search_room_info";
					} 
					else {
						ret.value = failed;
						ret.ret_type = "search_room_info";
						ret.ret_str = "Room not found";
					}
				}
				else if (gmsg.get_type == "room_info") {
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
							if (rid > 0 && rid < rooms.size()) {
								infos.push_back(static_cast<room_info>(*rooms[rid]));
							}
							else {
								infos.push_back(room_info(0));
							}
						}
					}
					struct_json::to_json(infos, ret.ret_str);
					ret.value = success;
					ret.ret_type = "room_info";
				}
				else if (gmsg.get_type == "user_info") {
					std::vector<basic_user> infos;
					for (auto& gid : gmsg.ids) {
						if (auto gp = find_user(gid); gp != nullptr) {
							infos.push_back(*gp);
						}
					}
					if (!infos.empty()) {
						struct_json::to_json(infos, ret.ret_str);
						ret.value = success;
						ret.ret_type = "user_info";
					}
					else {
						ret.value = failed;
						ret.ret_type = "get_falied";
						ret.ret_str = "User not found";
					}
				}
				else if (gmsg.get_type == "notices") {
					struct_json::to_json(msg_queue, ret.ret_str);
					ret.value = success;
					ret.ret_type = "notices";
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
		virtual void leave(basic_user_ptr p) override {
			user_id_left.push(p->id);
			room::leave(p);
		}
		void notice(std::string s) {
			deliver(wrap(notice_msg{ 
				.str = s,
				}, msg_t::notice));
		}
		virtual ~hall() = default;
	private:
		boost::asio::awaitable<void> listener(boost::asio::ip::tcp::acceptor acceptor) {
			for (;;){
				user_ptr g = std::make_shared<user>(
					co_await acceptor.async_accept(boost::asio::use_awaitable),
					shared_from_this(),
					-1
				);
				g->id = distribute_user_id();
				g->start();
			}
		}

		std::vector<room_ptr> rooms;
		std::priority_queue<int> dissolved_room_id, user_id_left;
		std::shared_ptr<boost::asio::io_context> context_ptr_;
	};
}
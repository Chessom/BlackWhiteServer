#include"user.h"
bw::server::user::user(socket_t socket, room_ptr room_p, int user_id)
	:sock(std::move(socket)), room_(room_p), hall_(room_p), timer_(sock.get_executor())
{
	id = user_id;
	timer_.expires_at(std::chrono::steady_clock::time_point::max());
}

void bw::server::user::deliver(const message& msg)
{
	write_msg_queue.push_back(msg);
	timer_.cancel_one();
}

void bw::server::user::broadcast(const message& msg)
{
	if (room_ != nullptr) {
		room_->deliver(msg);
	}
}

void bw::server::user::start()
{
	hall_->join(shared_from_this());
	boost::asio::co_spawn(sock.get_executor(), [self = shared_from_this()] {return self->reader(); }, boost::asio::detached);
	boost::asio::co_spawn(sock.get_executor(), [self = shared_from_this()] {return self->writer(); }, boost::asio::detached);
	//deliver(wrap(control_msg{
	//	.type=control_msg::update,
	//	.content="Welcome to Server 001!",
	//	.id1=id,
	//	.target_type=control_msg::g
	//	}, msg_t::control));
	spdlog::info("ID:{} Gamer connects the server.", id);
}

void bw::server::user::join(room_ptr room_p)
{
	if (room_p == room_)return;
	if (room_ != hall_ && room_ != nullptr) {
		leave();
	}
	room_ = room_p;
	room_->join(shared_from_this());
	room_->deliver(wrap(control_msg
		{ 
			.type = control_msg::join,
			.id1 = id,
			.id2 = room_->id 
		}, message::control));
}

void bw::server::user::leave()
{
	auto p = shared_from_this();
	if (room_->is_default()) {//没加入其他房间
		room_->leave(p);//此时hall_==room_
		room_ = nullptr;
		stop();
		return;
	}
	room_->deliver(wrap(control_msg
		{
			.type = control_msg::leave,
			.id1 = id,
			.id2 = room_->id 
		}, msg_t::control));
	room_->leave(p);
	room_ = hall_;
}

void bw::server::user::stop()
{
	sock.close();
	timer_.cancel();
	logout();
}

void bw::server::user::home()
{
	if (!room_->is_default()) {
		leave();
	}
}

void bw::server::user::logout()
{
	auto p = shared_from_this();
	if (room_ != nullptr) {
		if (room_->is_default()) {
			hall_->leave(p);
		}
		else {
			room_->leave(p);
			hall_->leave(p);
		}
	}
	else {
		hall_->leave(p);
	}
}

void bw::server::user::handle_msg(const message& msg) {
	hall_->handle_msg(shared_from_this(), msg);
}

boost::asio::awaitable<void> bw::server::user::reader()
{
	using namespace boost::asio;
	std::string read_str, sizestr;
	msg_t msg;
	try {
		while (true) {
			co_await async_read(sock, dynamic_buffer(sizestr, sizelen), use_awaitable);
			co_await async_read(sock, dynamic_buffer(read_str, std::stoi(sizestr)), use_awaitable);
			struct_json::from_json(msg, read_str);
			handle_msg(msg);
			sizestr = "";
			read_str = "";
		}
	}
	catch (const std::exception& e) {
		spdlog::info("ID:{} name:{} disconnect the server.", id, name);
		stop();
	}
}

boost::asio::awaitable<void> bw::server::user::writer()
{
	using namespace boost::asio;
	try
	{
		std::string write_str;
		size_t size;
		while (sock.is_open())
		{
			if (write_msg_queue.empty())
			{
				try {
					boost::system::error_code ec;
					co_await timer_.async_wait(redirect_error(use_awaitable, ec));
				}
				catch (const std::exception&){}
			}
			else
			{
				write_str = "";
				iguana::to_json(write_msg_queue.front(), write_str);
				co_await boost::asio::async_write(sock, 
					boost::asio::buffer(std::format("{0:<{1}}", write_str.size(), sizelen), sizelen), use_awaitable);
				co_await boost::asio::async_write(sock,
					boost::asio::buffer(write_str, write_str.size()), use_awaitable);
				write_msg_queue.pop_front();
			}
		}
	}
	catch (std::exception& e)
	{
		spdlog::info("ID:{} name:{} disconnect the server.", id, name);
		stop();
	}
}

//	int type = msg.type;
//	if (type == msg_t::invalid)
//		throw std::runtime_error("Invalid Control Message");
//	if (type == msg_t::control) {
//		control_msg con_m;
//		struct_json::from_json(con_m, msg.jsonstr);
//		if (con_m.type == control_msg::create) {
//			auto hall_p = std::dynamic_pointer_cast<hall>(hall_);
//			msg_t ret;
//			if (con_m.target_type == control_msg::r) {
//				room_info info;
//				int newid = hall_p->create_room(con_m.content);
//				struct_json::from_json(info, con_m.content);
//				hall_p->rooms[newid]->load_info(info);
//				ret = wrap(ret_msg{ .value = success ,.ret_str = std::to_string(newid) }, msg_t::ret);
//			}
//			else {
//				ret = wrap(ret_msg{ .value = failed,.ret_str = "Invalid target type" }, msg_t::ret);
//			}
//			deliver(ret);
//		}
//		else if(con_m.type == control_msg::update){//修改room等的info id1为user id,id2为room id;
//			auto hall_p = std::dynamic_pointer_cast<hall>(hall_);
//			msg_t ret;
//			if (con_m.target_type == control_msg::g) {
//				basic_user ginfo;
//				if (con_m.id1 == id) {
//					struct_json::from_json(ginfo, con_m.content);
//					load_info(ginfo);
//					ret = wrap(ret_msg{ .value = success }, msg_t::ret);
//				}
//				else if (hall_p->find_user(con_m.id1)->authority == admin) {
//					for (auto& g : hall_p->users) {
//						if (g->id == con_m.id1) {
//							struct_json::from_json(ginfo, con_m.content);
//							g->load_info(ginfo);
//							break;
//						}
//					}
//					ret = wrap(ret_msg{ .value = success }, msg_t::ret);
//				}
//				else {
//					ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
//				}
//			}
//			else if (con_m.target_type == control_msg::r) {
//				room_info rinfo;
//				if (hall_p->rooms[con_m.id2]->owner == con_m.id1 || hall_p->find_user(con_m.id1)->authority == admin) {
//					struct_json::from_json(rinfo, con_m.content);
//					hall_p->rooms[con_m.id2]->load_info(rinfo);
//					ret = wrap(ret_msg{ .value = success }, msg_t::ret);
//				}
//				else
//					ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
//			}
//			else {
//				ret = wrap(ret_msg{ .value = failed,.ret_str = "FATAL:Data structure damaged" }, msg_t::ret);
//				spdlog::error("Data structure damaged;id:{}", id);
//			}
//			deliver(ret);
//		}
//		else if (con_m.type == control_msg::del) {
//			auto hall_p = std::dynamic_pointer_cast<hall>(hall_);
//			msg_t ret;
//			if (con_m.target_type == control_msg::r) {
//				if (hall_p->find_user(con_m.id1)->authority == admin) {
//					auto& temp_room_p = hall_p->rooms[con_m.id2];
//					if (temp_room_p != nullptr) {
//						temp_room_p->dissolve();
//						temp_room_p = nullptr;
//						ret = wrap(ret_msg{ .value = success }, msg_t::ret);
//					}
//					else
//					{
//						ret = wrap(ret_msg{ .value = failed,.ret_str = "Room not exist" }, msg_t::ret);
//					}
//				}
//				else {
//					if (hall_p->rooms[con_m.id2] != nullptr) {
//						if (hall_p->rooms[con_m.id2]->owner == con_m.id1) {
//							hall_p->rooms[con_m.id2]->dissolve();
//							hall_p->rooms[con_m.id2] = nullptr;
//							ret = wrap(ret_msg{ .value = success }, msg_t::ret);
//						}
//						else {
//							ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
//						}
//					}
//					else {
//						ret = wrap(ret_msg{ .value = failed,.ret_str = "Room not exist" }, msg_t::ret);
//					}
//				}
//			}
//			else if(con_m.target_type == control_msg::g){
//				user_ptr temp_p = nullptr;
//				if (con_m.id1 == id) {
//					temp_p = shared_from_this();
//					ret = wrap(ret_msg{ .value = success }, msg_t::ret);
//					temp_p->deliver(ret);
//					if (temp_p->room_->is_default()) {
//						temp_p->leave();
//					}
//					else {
//						temp_p->leave();
//						temp_p->leave();
//					}
//				}
//				else if (hall_p->find_user(con_m.id1)->authority == admin) {
//					for (auto& g : hall_p->users) {
//						if (g->id == con_m.id1) {
//							temp_p = std::dynamic_pointer_cast<user>(g);
//							break;
//						}
//					}
//					temp_p->deliver(wrap(str_msg{ .content = "You have been kicked out of server!",.id1 = id }, msg_t::str));
//					if (temp_p->room_->is_default()) {
//						temp_p->leave();
//					}
//					else {
//						temp_p->leave();
//						temp_p->leave();
//					}
//				}
//				else {
//					ret = wrap(ret_msg{ .value = failed,.ret_str = "No permission" }, msg_t::ret);
//				}
//			}
//			else {
//				ret = wrap(ret_msg{ .value = failed,.ret_str = "Invalid target type" }, msg_t::ret);
//			}
//		}
//		else if (con_m.type == control_msg::join) {
//			auto hall_p = std::dynamic_pointer_cast<hall>(hall_);
//			auto gp = std::dynamic_pointer_cast<user>(hall_p->find_user(con_m.id1));
//			gp->join(hall_p->rooms[con_m.id2]);
//			deliver(wrap(ret_msg{ .value = success }, msg_t::ret));
//		}
//		else if (con_m.type == control_msg::leave) {
//			auto hall_p = std::dynamic_pointer_cast<hall>(hall_);
//			auto gp = std::dynamic_pointer_cast<user>(hall_p->find_user(con_m.id1));
//			gp->leave();
//			deliver(wrap(ret_msg{ .value = success }, msg_t::ret));
//		}
//		else if (con_m.type == control_msg::none) {
//			deliver(wrap(ret_msg{ .value = success,.ret_str = "Always success" }, msg_t::ret));
//		}
//		else {
//			deliver(wrap(ret_msg{ .value = failed,.ret_str = "FATAL:Data structure damaged" }, msg_t::ret));
//			spdlog::error("Data structure damaged;id:{}", con_m.id1);
//		}
//	}
//	else if (type == msg_t::str) {
//		str_msg strmsg;
//		struct_json::from_json(strmsg, msg.jsonstr);
//		auto hall_p = std::dynamic_pointer_cast<hall>(hall_);
//		auto gp = std::dynamic_pointer_cast<user>(hall_p->find_user(strmsg.id1));
//		if (strmsg.target_type == str_msg::g) {
//			gp->deliver(wrap(str_msg{ .content = strmsg.content,.id1 = id }, msg_t::str));
//		}
//		else {
//			if (gp->room_->is_default()) {
//				if (gp->authority == admin) {
//					gp->broadcast(wrap(strmsg, msg_t::str));
//				}
//				else {
//					gp->deliver(wrap(ret_msg{ .value = failed,.ret_str = "Only administrators can broadcast messages in whole server." }, msg_t::ret));
//				}
//			}
//			else {
//				gp->broadcast(wrap(strmsg, msg_t::str));
//			}
//		}
//	}
//	else if (type == msg_t::game) {
//		game_msg gmsg;
//		struct_json::from_json(gmsg, msg.jsonstr);
//		if (room_->is_default()) {
//			auto st = room_->state;
//			if (st == room_info::prepared || st == room_info::gaming) {
//				room_->deliver(wrap(gmsg, msg_t::game));
//			}
//		}
//	}
//	else {
//		deliver(wrap(ret_msg{ .value = failed,.ret_str = "FATAL:Data structure damaged" }, msg_t::ret));
//		spdlog::error(msg.jsonstr);
//	}
//}
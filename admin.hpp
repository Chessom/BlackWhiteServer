#pragma once
#include"gamer.h"
#include"hall.hpp"
namespace bw::server {
	class admin_gamer :public gamer {
		admin_gamer(socket_t socket, room_ptr room_p) :gamer(std::move(socket), room_p, 0) {
			id = 0;
			authority = admin;
			name = "Administrator";
		};
		virtual void deliver(const message&) {
			
		}
		virtual void broadcast(const message&) {
			
		}
		virtual void start() {

		}
		virtual void join(room_ptr room_p) {

		}
		virtual void leave() {

		}
		virtual void stop() {

		}
		virtual void home() {

		}
	private:
		virtual inline void handle_msg(const message&) {

		}
		virtual ~admin_gamer() = default;
	};
}
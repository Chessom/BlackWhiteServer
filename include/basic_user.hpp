#pragma once
#include"message.hpp"
#include<print>
namespace bw::server {
	namespace user_st {
		enum { prepared, gaming, free, watching };
	}
	class basic_user {
	public:
		virtual void deliver(const message& msg) { std::println("deliver Unimplemented."); };
		virtual void home() { std::println("home Unimplemented."); };
		inline void load_info(const basic_user& info)
		{
			name = info.name;
			id = info.id;
			state = info.state;
			authority = info.authority;
		}
		virtual ~basic_user() {}
		std::string name;
		int state = user_st::free;
		int id = -1;
		int authority = anonymous;
		enum { admin_id = 0 };
		enum { admin, ordinary, anonymous, limited };
	};
	using basic_user_ptr = std::shared_ptr<basic_user>;
	REFLECTION(basic_user, name, id, state, authority);
}
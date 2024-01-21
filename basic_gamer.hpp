#pragma once
#include"message.hpp"
namespace bw::server {
	namespace gamer_st {
		enum { prepared, gaming, free };
	}
	class basic_gamer {
	public:
		virtual void deliver(const message& msg) {};
		virtual void home() {};
		inline void load_info(const basic_gamer& info)
		{
			name = info.name;
			id = info.id;
			state = info.state;
			authority = info.authority;
		}
		virtual ~basic_gamer() {}
		std::string name;
		int state = gamer_st::free;
		int id = -1;
		int authority = ordinary;
		enum { admin, ordinary, anonymous, limited };
	};
	using basic_gamer_ptr = std::shared_ptr<basic_gamer>;
	REFLECTION(basic_gamer, name, id, state, authority);
}
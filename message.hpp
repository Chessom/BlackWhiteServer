#pragma once
#include"stdafx.h"
namespace bw {
	struct control_msg {
		enum { create, update, del, leave, join, none };
		int type = none;
		std::string content;
		int id1, id2;
		//ID1用来表示用户，ID2表示roomID
		int target_type;
		enum { r, g, n };
	};
	REFLECTION(control_msg, type, content, id1, id2, target_type);
	struct str_msg {
		std::string content;
		int target_type = g;
		int id1, id2;
		enum { r, g, n };
	};
	REFLECTION(str_msg, content, target_type, id1, id2);
	struct game_msg {
		int mode;
		std::string movestr, board;
	};
	REFLECTION(game_msg, mode, movestr, board);
	enum { success, failed };
	struct ret_msg {
		int value = failed;
		std::string ret_str;
	};
	REFLECTION(ret_msg, value, ret_str);

	class message {
	public:
		enum { invalid, str, game, control, ret };
		message() = default;
		template<typename T>
		message(T&& obj, int message_type = invalid) {
			iguana::refletable_v<control_msg>;
			iguana::to_json(std::forward<T>(obj), jsonstr);
			type = message_type;
		}
		template<typename T>
		void reload(T&& obj, int message_type = invalid) {
			jsonstr = "";
			type = message_type;
			iguana::to_json(std::forward<T>(obj), jsonstr);
		}
		std::string jsonstr;
		int type = invalid;
	};
	REFLECTION(message, jsonstr, type);
	using msg_t = message;
}
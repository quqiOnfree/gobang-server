#include "local_class.hpp"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <json/json.h>
#include <boost/any.hpp>

#include <mysql.h>
#pragma comment(lib, "libmysql.lib")

#include "log_system.hpp"
#include "qmath.hpp"
#include "headmsg.hpp"

using namespace std;

using pair_void = pair<string, Json::Value(*)(Json::Value, SOCKET conn, sockaddr_in addr)>;
using voidpair_void = pair<string, boost::any(*)(Json::Value, SOCKET conn, sockaddr_in addr)>;

extern vector<pair<string, Json::Value(*)(Json::Value, SOCKET, sockaddr_in)>> funcs;
extern vector<pair<string, Json::Value(*)(Json::Value, SOCKET, sockaddr_in)>> voidfuncs;
extern log_system lg;
extern MYSQL* mysql;

class User
{
private:
	string username_;
	SOCKET sock_;
	sockaddr_in addr_;
	bool is_exist_;
public:
	User()
	{
		is_exist_ = false;
		sock_ = NULL;
		memset(&addr_, 0, sizeof(addr_));
	}
	User(string username,SOCKET s, sockaddr_in addr)
	{
		username_ = username;
		memcpy(&sock_, &s, sizeof(s));
		memcpy(&addr_, &addr, sizeof(addr));
		is_exist_ = true;
	}
	void set_exist(bool bo)
	{
		is_exist_ = bo;
	}
	void update(string str)
	{
		char* strs = new char[str.size() + 8 + 1]{ 0 };
		size_t len = headmsg::makeHeadCStr(str.c_str(), str.size(), strs, str.size() + 8 + 1);
		//lg.write(vstype({ "server>>", inet_ntoa(addr_.sin_addr), ":", to_string(short(ntohs(addr_.sin_port))).c_str(), ">>", str }));
		if (send(sock_, strs, len, 0) == SOCKET_ERROR)
			is_exist_ = false;
		delete[] strs;
	}
	bool get_exist()
	{
		return is_exist_;
	}
	SOCKET get_socket()
	{
		return sock_;
	}
	sockaddr_in get_addr()
	{
		return addr_;
	}
	string get_name()
	{
		return username_;
	}

	bool operator ==(User& user)
	{
		if (this == &user)
		{
			return true;
		}
		return false;
	}

	bool operator ==(const User& user)
	{
		if (this == &user)
		{
			return true;
		}
		return false;
	}
};

class baseRoom
{
private:
	vector<User*> user_;
public:
	baseRoom() :user_() {}
	~baseRoom() {}
	bool addUser(User& user)
	{
		for (auto i = user_.begin(); i != user_.end(); i++)
		{
			if (*(*i) == user) return false;
		}
		user_.push_back(&user);
		return true;
	}
	bool removeUser(User& user)
	{
		if (user_.size() == 0) return false;
		for (auto i = user_.begin(); i != user_.end(); i++)
		{
			if (*(*i) == user)
			{
				user_.erase(i);
				return true;
			}
		}
		return false;
	}
	size_t userSize() const
	{
		return user_.size();
	}
	void motifyUser(Json::Value str__)
	{
		if (user_.size() == 0LL)return;
		for (auto i = user_.begin(); i != user_.end(); i++)
		{
			(*i)->update(Json::FastWriter().write(str__));
		}
	}
	void clear() { user_.clear(); }
	bool isExist(User& user)
	{
		if (user_.size() == 0LL) return false;
		for (auto i = user_.begin(); i != user_.end(); i++)
		{
			if (*(*i) == user) return true;
		}
		return false;
	}
};

map<string, User> users;

class room:public baseRoom
{
private:
	string roomname_;
	string hostname_;
	string theother_;
	size_t member_;
	size_t max_member_;
	char chessborad_[19][19][20]{ 0 };
	bool gameStart_;
	bool gameover_;
	bool can_regret;
	string state_;
	map<string, string> piece_;
	string now_piece_;
	string winner_;
	vector<tuple<int, int, string>> order_;

protected:
	void start()
	{
		gameStart_ = true;
		state_ = "已开始";
		Json::Value re;
		re["start"] = true;
		if (randint(0, 1))
		{
			piece_[hostname_] = "black";
			piece_[users[theother_].get_name()] = "white";
		}
		else
		{
			piece_[hostname_] = "white";
			piece_[users[theother_].get_name()] = "black";
		}
		now_piece_ = "black";
		motifyUser(re);
	}

public:
	room()
	{
		max_member_ = 2;
		member_ = 1;
		state_ = "等待中";
		for (int i = 0; i < 19; i++)
		{
			for (int j = 0; j < 19; j++)
			{
				sprintf_s(chessborad_[i][j], ".");
			}
		}
		gameStart_ = false;
		gameover_ = false;
		can_regret = false;
	}
	room(string roomname,string hostname,User& user)
	{
		addUser(user);
		roomname_ = roomname;
		hostname_ = hostname;
		max_member_ = 2;
		member_ = 1;
		state_ = "等待中";
		for (int i = 0; i < 19; i++)
		{
			for (int j = 0; j < 19; j++)
			{
				sprintf_s(chessborad_[i][j], ".");
			}
		}
		gameStart_ = false;
		gameover_ = false;
		can_regret = false;
	}
	bool add_user(User& user)
	{
		if (member_ < max_member_)
		{
			
			if (addUser(user))
			{
				member_++;
				theother_ = user.get_name();
				if (member_ == max_member_) start();
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	bool handle_event(string user,unsigned short x, unsigned short y)
	{
		if (gameStart_)
		{
			if (x >= 19 || y >= 19) return false;
			{
				bool is_user = false;
				for (auto i = piece_.begin(); i != piece_.end(); i++)
				{
					if (i->first == user)
					{
						is_user = true;
						break;
					}
				}
				if (!is_user) return false;
			}

			if (now_piece_ != piece_[user])
				return false;
			
			{
				if (string(".") == chessborad_[x][y])
				{
					strcpy(chessborad_[x][y], piece_[user].c_str());

					order_.emplace_back(x, y, piece_[user]);

					if (piece_[user] == "black")
						now_piece_ = "white";
					else now_piece_ = "black";

					return true;
				}
				else return false;
			}
		}
		else return false;
	}
	int get_continuous_count(unsigned short r, unsigned short c, int dr, int dc)
	{
		string piece = chessborad_[r][c];
		int result = 0;
		int i = 1;
		int new_r = 0;
		int new_c = 0;
		while (true)
		{
			new_r = r + dr * i;
			new_c = c + dc * i;
			if ((0 <= new_r && new_r < 19) && (0 <= new_c && new_c < 19))
			{
				if (piece == chessborad_[new_r][new_c])
					result++;
				else break;
			}
			else break;
			i++;
		}
		return result;
	}
	void check_win(unsigned short r, unsigned short c)
	{
		int n_count = get_continuous_count(r, c, -1, 0);
		int s_count = get_continuous_count(r, c, 1, 0);
		int w_count = get_continuous_count(r, c, 0, -1);
		int e_count = get_continuous_count(r, c, 0, 1);
		int nw_count = get_continuous_count(r, c, -1, -1);
		int ne_count = get_continuous_count(r, c, -1, 1);
		int sw_count = get_continuous_count(r, c, 1, -1);
		int se_count = get_continuous_count(r, c, 1, 1);
		if (n_count + s_count + 1 >= 5 || e_count + w_count + 1 >= 5 || se_count + nw_count + 1 >= 5 || ne_count + sw_count + 1 >= 5)
		{
			winner_ = chessborad_[r][c];
			gameover_ = true;
			Json::Value re;
			re["gameover"] = true;
			re["winner"] = winner_;
			{
				for (auto i = piece_.begin(); i != piece_.end(); i++)
				{
					if (i->second == winner_)
					{
						{
							char str[1024]{ 0 };
							sprintf_s(str, "update user set socket_win_game = socket_win_game + 1 where name = '%s'", i->first.c_str());
							mysql_query(mysql, str);
						}
						{
							char str[1024]{ 0 };
							sprintf_s(str, "update user set level = level + 1 where name = '%s'", i->first.c_str());
							mysql_query(mysql, str);
						}
						{
							char str[1024]{ 0 };
							sprintf_s(str, "update user set score = score + %s where name = '%s'", to_string(order_.size()).c_str(), i->first.c_str());
							mysql_query(mysql, str);
						}
						break;
					}
				}
			}
			motifyUser(re);
		}
	}
	Json::Value get_chessboard() const
	{
		Json::Value re;
		Json::Value re2;
		for (int i = 0; i < 19; i++)
		{
			for (int j = 0; j < 19; j++)
			{
				re2.append(chessborad_[i][j]);
			}
			re.append(re2);
			re2.clear();
		}
		return re;
	}
	bool regret_piece(Json::Value value)
	{
		if (value["name"].isString())
		{
			{
				bool is_exist = false;
				for (auto i = piece_.begin(); i != piece_.end(); i++)
				{
					if (i->first == value["name"].asString())
					{
						is_exist = true;
						break;
					}
				}
				if (!is_exist) return false;
			}

			if (order_.size() != 0)
			{
				if (get<2>(order_.back()) == piece_[value["name"].asString()])
				{
					strcpy(chessborad_[get<0>(order_.back())][get<1>(order_.back())], ".");
					if (get<2>(order_.back()) == "black")
					{
						now_piece_ = "black";
					}
					else
					{
						now_piece_ = "white";
					}
					order_.pop_back();
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	bool leave_room(Json::Value value)
	{
		if (value["name"].isString())
		{
			if (gameStart_)
			{
				bool is_exist = false;
				for (auto i = piece_.begin(); i != piece_.end(); i++)
				{
					if (i->first == value["name"].asString())
					{
						is_exist = true;
						break;
					}
				}
				if (!is_exist) return false;
			}

			{
				Json::Value re;
				re["leaveroom"] = "有玩家退出！";
				motifyUser(re);
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	string get_state() const
	{
		return state_;
	}
	string get_roomname() const
	{
		return roomname_;
	}
	string get_hostname() const
	{
		return hostname_;
	}
	bool get_gameover() const
	{
		return gameover_;
	}
	string get_winner() const
	{
		return winner_;
	}
	string get_nowpiece() const
	{
		return now_piece_;
	}
	map<string, string> get_piece() const
	{
		return piece_;
	}
	vector<tuple<int, int, string>> get_order() const
	{
		return order_;
	}
};

map<string, room> rooms;

Json::Value get_yourpiece(Json::Value value, SOCKET conn, sockaddr_in addr)
{
	Json::Value re;
	if (value["name"].isString())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "房间不存在或用户不存在！可能是房主已退出";
				return re;
			}
		}

		{
			re["yourpiece"] = i->second.get_piece()[value["name"].asString()];
			return re;
		}
	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

Json::Value fall_piece(Json::Value value, SOCKET conn, sockaddr_in addr) //下棋
{
	Json::Value re;
	if (value["name"].isString() && value["x"].isInt() && value["y"].isInt())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "房间不存在或用户不存在！可能是房主已退出";
				return re;
			}
		}

		{
			if (i->second.handle_event(value["name"].asString(), value["x"].asInt(), value["y"].asInt()))
			{
				i->second.check_win(value["x"].asInt(), value["y"].asInt());
				re["success"] = "下棋成功！";
				return re;
			}
			else
			{
				re["error"] = "不允许下棋！可能是此位置已经有棋子 或者 游戏已经结束";
				return re;
			}
		}
		
	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

Json::Value get_nowpiece(Json::Value value, SOCKET conn, sockaddr_in addr) //获取现在下棋的选手
{
	Json::Value re;
	if (value["name"].isString())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "房间不存在或用户不存在！可能是房主已退出";
				return re;
			}
		}

		{
			re["nowpiece"] = i->second.get_nowpiece();
			return re;
		}
	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

Json::Value get_chessboard(Json::Value value, SOCKET conn, sockaddr_in addr) //获取棋盘
{
	Json::Value re;
	if (value["name"].isString())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "房间不存在或用户不存在！可能是房主已退出";
				return re;
			}
		}

		{
			re["chessboard"] = i->second.get_chessboard();
			return re;
		}
	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

Json::Value get_lastpiece(Json::Value value, SOCKET conn, sockaddr_in addr) //获取上一个点
{
	Json::Value re;
	if (value["name"].isString())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "没有此用户！";
				return re;
			}
		}

		if (i->second.get_order().size() != 0)
		{
			re["lastpiece"]["x"] = get<0>(i->second.get_order().back());
			re["lastpiece"]["y"] = get<1>(i->second.get_order().back());
			return re;
		}
		else
		{
			re["error"] = "没有下棋！";
			return re;
		}
	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

Json::Value leave_room(Json::Value value, SOCKET conn, sockaddr_in addr) //离开房间
{
	Json::Value re;
	if (value["name"].isString())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "房间不存在或用户不存在！可能是房主已退出";
				return re;
			}
		}

		{
			if (i->second.leave_room(value))
			{
				{
					bool is_exist = false;
					for (auto i = rooms.begin(); i != rooms.end(); i++)
					{
						if (i->second.isExist(users[value["name"].asString()]))
						{
							rooms.erase(i);
							is_exist = true;
							break;
						}
					}
					if (!is_exist)
					{
						re["error"] = "无法退出！";
						return re;
					}
				}
				re["success"] = true;
				return re;
			}
			else
			{
				re["error"] = "无法退出！";
				return re;
			}
		}
	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

Json::Value regret_piece(Json::Value value, SOCKET conn, sockaddr_in addr) //悔棋
{
	Json::Value re;
	if (value["name"].isString())
	{
		auto i = rooms.begin();
		{
			bool is_exist = false;
			for (; i != rooms.end(); i++)
			{
				if (i->second.isExist(users[value["name"].asString()]))
				{
					is_exist = true;
					break;
				}
			}
			if (!is_exist)
			{
				re["error"] = "房间不存在或用户不存在！可能是房主已退出";
				return re;
			}
		}
		
		{
			if (i->second.regret_piece(value))
			{
				re["success"] = true;
				return re;
			}
			else
			{
				re["error"] = false;
				return re;
			}
		}

	}
	else
	{
		re["error"] = "参数不足或参数类型不正确！";
		return re;
	}
}

//Json::Value print_json(Json::Value value, SOCKET conn, sockaddr_in addr)//示例函数
//{
//	cout << Json::FastWriter().write(value).substr(0, Json::FastWriter().write(value).length() - 1) << endl;
//	Json::Value _va;
//	_va["result"] = "success";
//	return _va;
//}

Json::Value is_on(Json::Value value, SOCKET conn, sockaddr_in addr) //监测服务器是否开启
{
	Json::Value re;
	return re = true;
}

Json::Value Register(Json::Value value, SOCKET conn, sockaddr_in addr) //注册
{
	if (!value["name"].isNull() && !value["password"].isNull())
	{
		if (value["name"].isString() && value["password"].isString())
		{
			{
				if (value["name"].asString().size() != 0 && value["password"].asString().size() != 0)
				{
					string name = value["name"].asString();

					for (auto i = name.begin(); i != name.end(); i++)
					{
						if (*i == '?' || *i == '\'' || *i == ';' || *i == ':' || *i == '#')
						{
							Json::Value re;
							re["error"] = "不允许存在非法字符";
							return re;
						}
					}

					string password = value["password"].asString();
					for (auto i = password.begin(); i != password.end(); i++)
					{
						if (*i == '?' || *i == '\'' || *i == ';' || *i == ':' || *i == '#')
						{
							Json::Value re;
							re["error"] = "不允许存在非法字符";
							return re;
						}
					}
				}
				else
				{
					Json::Value re;
					re["error"] = "不允许空字符";
					return re;
				}
			}
			{
				char str[1024]{ 0 };
				sprintf_s(str, "select count(name) from user where name = '%s'", value["name"].asCString());
				mysql_query(mysql, str);
				MYSQL_RES* res = mysql_store_result(mysql);
				char** row;
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				if (string(row[0]) != "0")
				{
					Json::Value re;
					re["error"] = "此用户已经注册！";
					return re;
				}
			}
			long long id = 0;
			while (true)
			{
				id = randint(1000, 8388607);
				char str[1024]{ 0 };
				sprintf_s(str, "select count(id) from user where id = %s", to_string(id).c_str());
				mysql_query(mysql, str);
				MYSQL_RES* res = mysql_store_result(mysql);
				char** row;
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				if (string(row[0]) == "0")
				{
					break;
				}
			}
			char str[1024]{ 0 };
			sprintf_s(str, "insert into user values (%s,'%s','%s',1,0,0,0,NOW(),NOW())", to_string(id).c_str(), value["name"].asCString(), value["password"].asCString());
			if (mysql_query(mysql, str))
				return (Json::Value()["error"] = "注册失败！");
			else 
			{
				Json::Value re;
				re["success"] = "注册成功！";
				return re;
			}
		}
		else
		{
			Json::Value re;
			re["error"] = "参数类型不正确！";
			return re;
		}
	}
	else
	{
		Json::Value re;
		re["error"] = "参数不足！";
		return re;
	}
}

Json::Value login(Json::Value value, SOCKET conn, sockaddr_in addr) //登录
{
	if (!value["name"].isNull() && !value["password"].isNull())
	{
		if (value["name"].isString() && value["password"].isString())
		{
			{
				if (value["name"].asString().size() != 0 && value["password"].asString().size() != 0)
				{
					string name = value["name"].asString();

					for (auto i = name.begin(); i != name.end(); i++)
					{
						if (*i == '?' || *i == '\'' || *i == ';' || *i == ':' || *i == '#')
						{
							Json::Value re;
							re["error"] = "不允许存在非法字符";
							return re;
						}
					}

					string password = value["password"].asString();
					for (auto i = password.begin(); i != password.end(); i++)
					{
						if (*i == '?' || *i == '\'' || *i == ';' || *i == ':' || *i == '#')
						{
							Json::Value re;
							re["error"] = "不允许存在非法字符";
							return re;
						}
					}
				}
				else
				{
					Json::Value re;
					re["error"] = "不允许空字符";
					return re;
				}
			}

			{
				char str[1024]{ 0 };
				sprintf_s(str, "select count(name) from user where name = '%s'", value["name"].asCString());
				mysql_query(mysql, str);
				MYSQL_RES* res = mysql_store_result(mysql);
				char** row;
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				if (string(row[0]) == "0")
				{
					Json::Value re;
					re["error"] = "没有此用户！";
					return re;
				}
			}

			{
				char str[1024]{ 0 };
				sprintf_s(str, "select password from user where name = '%s'", value["name"].asCString());
				mysql_query(mysql, str);
				MYSQL_RES* res = mysql_store_result(mysql);
				char** row;
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				if (string(row[0]) == value["password"].asString())
				{
					{
						for (auto i = users.begin(); i != users.end(); i++)
						{
							if (i->first == value["name"].asString())
							{
								Json::Value re;
								re["error"] = "用户已登录！";
								return re;
							}
						}
					}
					{
						char str[1024]{ 0 };
						sprintf_s(str, "update user set login_time = NOW() where name = '%s'", value["name"].asCString());
						mysql_query(mysql, str);
					}
					Json::Value re;
					re["success"] = "登录成功";
					users[value["name"].asString()] = User(value["name"].asString(), conn, addr);
					return re;
				}
				else
				{
					Json::Value re;
					re["error"] = "密码错误！";
					return re;
				}
			}
		}
		else
		{
			Json::Value re;
			re["error"] = "参数类型不正确！";
			return re;
		}
	}
	else
	{
		Json::Value re;
		re["error"] = "参数不足！";
		return re;
	}
}

Json::Value get_data(Json::Value value, SOCKET conn, sockaddr_in addr) //获取房间
{
	if (!value["name"].isNull())
	{
		if (value["name"].isString())
		{
			{
				char str[1024]{ 0 };
				sprintf_s(str, "select count(name) from user where name = '%s'", value["name"].asCString());
				mysql_query(mysql, str);
				MYSQL_RES* res = mysql_store_result(mysql);
				char** row;
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				if (string(row[0]) == "0")
				{
					Json::Value re;
					re["error"] = "没有此用户！";
					return re;
				}
			}

			{
				char str[1024]{ 0 };
				sprintf_s(str, "select * from user where name = '%s'", value["name"].asCString());
				mysql_query(mysql, str);
				MYSQL_RES* res = mysql_store_result(mysql);
				char** row;
				row = mysql_fetch_row(res);
				mysql_free_result(res);
				Json::Value re;
				re["name"] = row[1];
				re["level"] = row[3];
				re["score"] = row[4];
				re["socket_win_game"] = row[5];
				re["ai_win_game"] = row[6];
				re["reg_time"] = row[7];
				re["login_time"] = row[8];
				return re;
			}
		}
		else
		{
			Json::Value re;
			re["error"] = "参数类型不正确！";
			return re;
		}
	}
	else
	{
		Json::Value re;
		re["error"] = "参数不足！";
		return re;
	}
}

Json::Value get_room(Json::Value value, SOCKET conn, sockaddr_in addr) //获取房间
{
	Json::Value re;
	Json::Value local;
	bool is_run = false;
	for (auto i = rooms.begin(); i != rooms.end(); i++)
	{
		local["roomname"] = i->second.get_roomname();
		local["hostname"] = i->second.get_hostname();
		local["state"] = i->second.get_state();
		re.append(local);
		local.clear();
		is_run = true;
	}
	if (!is_run)
	{
		re = Json::Value(Json::arrayValue);
		return re;
	}
	return re;
}

Json::Value create_room(Json::Value value, SOCKET conn, sockaddr_in addr) //创建房间
{
	Json::Value re;
	if (!value["roomname"].isNull() && !value["hostname"].isNull())
	{
		if (value["roomname"].isString() && value["hostname"].isString())
		{
			bool is_find = false;
			for (auto i = users.begin(); i != users.end(); i++)
			{
				if (i->first == value["hostname"].asString())
				{
					is_find = true;
					break;
				}
			}
			if (!is_find)
			{
				re["error"] = "没有此用户！";
				return re;
			}
			rooms[value["roomname"].asString()] = room(value["roomname"].asString(), value["hostname"].asString(),users[value["hostname"].asString()]);
			
			re["success"] = "创建房间成功！";
			return re;
		}
		else
		{
			re["error"] = "参数类型不正确！";
			return re;
		}
	}
	else
	{
		re["error"] = "参数不足！";
		return re;
	}
}

Json::Value join_room(Json::Value value, SOCKET conn, sockaddr_in addr) //进入房间
{
	Json::Value re;
	if (!value["name"].isNull() && !value["hostname"].isNull())
	{
		if (value["name"].isString()&& value["hostname"].isString())
		{
			auto itor = rooms.begin();
			{
				bool is_exist = false;
				for (auto i = rooms.begin(); i != rooms.end(); i++)
				{
					if (i->second.get_hostname() == value["hostname"].asString())
					{
						is_exist = true;
						itor = i;
						break;
					}
				}
				if (!is_exist)
				{
					re["error"] = "没有此房间！";
					return re;
				}
			}

			{
				bool is_exist = false;
				for (auto i = users.begin(); i != users.end(); i++)
				{
					if (i->first == value["name"].asString())
					{
						is_exist = true;
						break;
					}
				}
				if (!is_exist)
				{
					re["error"] = "没有此用户！";
					return re;
				}
			}

			{
				if (itor->second.add_user(users[value["name"].asString()]))
				{
					re["success"] = "加入房间成功！";
					return re;
				}
				else
				{
					re["error"] = "加入房间失败！可能是房间人数已满";
					return re;
				}
			}
		}
		else
		{
			re["error"] = "参数类型不正确！";
			return re;
		}
	}
	else
	{
		re["error"] = "参数不足！";
		return re;
	}
}

Json::Value add_ai_score(Json::Value value, SOCKET conn, sockaddr_in addr)
{
	if (value["name"].isString())
	{
		{
			char str[1024]{ 0 };
			sprintf_s(str, "select count(name) from user where name = '%s'", value["name"].asCString());
			mysql_query(mysql, str);
			MYSQL_RES* res = mysql_store_result(mysql);
			char** row;
			row = mysql_fetch_row(res);
			mysql_free_result(res);
			if (string(row[0]) == "0")
			{
				Json::Value re;
				re["error"] = "没有此用户！";
				return re;
			}
		}

		//"update user set ai_win_game = ai_win_game + 1 where  name = 'quqi'";
		{
			char str[1024]{ 0 };
			sprintf_s(str, "update user set ai_win_game = ai_win_game + 1 where  name = '%s'", value["name"].asCString());
			mysql_query(mysql, str);
		}

		{
			char str[1024]{ 0 };
			sprintf_s(str, "update user set score = score + 10 where  name = '%s'", value["name"].asCString());
			mysql_query(mysql, str);
		}

		Json::Value re;
		re["success"] = "操作成功！";
		return re;
	}
	else
	{
		Json::Value re;
		re["error"] = "参数类型不正确！";
	}
}

void local_class_init()
{
	//funcs.push_back(pair_void(string("print_json"), print_json));
	funcs.push_back(pair_void(string("is_on"), is_on));
	funcs.push_back(pair_void(string("register"), Register));
	funcs.push_back(pair_void(string("login"), login));
	funcs.push_back(pair_void(string("get_data"), get_data));
	funcs.push_back(pair_void(string("get_room"), get_room));
	funcs.push_back(pair_void(string("create_room"), create_room));
	funcs.push_back(pair_void(string("get_nowpiece"), get_nowpiece));
	funcs.push_back(pair_void(string("get_yourpiece"), get_yourpiece));
	funcs.push_back(pair_void(string("get_chessboard"), get_chessboard));
	funcs.push_back(pair_void(string("get_lastpiece"), get_lastpiece));
	funcs.push_back(pair_void(string("add_ai_score"), add_ai_score));

	voidfuncs.push_back(pair_void(string("fall_piece"), fall_piece));
	voidfuncs.push_back(pair_void(string("join_room"), join_room));
	voidfuncs.push_back(pair_void(string("leave_room"), leave_room));
	voidfuncs.push_back(pair_void(string("regret_piece"), regret_piece));
}

void close_socket(SOCKET sock, sockaddr_in addr)
{
	for (auto i = rooms.begin(); i != rooms.end(); i++)
	{
		if (users[i->second.get_hostname()].get_socket() == sock)
		{
			rooms.erase(i);
			break;
		}
	}
	for (auto i = users.begin(); i != users.end(); i++)
	{
		if ((*i).second.get_socket() == sock)
		{
			users.erase(i);
			break;
		}
	}
}

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

//标准库
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <functional>
#include <map>
#include <future>

//自己写的库
#include "headmsg.hpp"
#include "inicpp.hpp"
#include "log_system.hpp"
#include "threadpool.hpp"
#include "local_class.hpp"

//第三方库
#include <json/json.h>
#include <boost/filesystem.hpp>

//socket库
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//mysql库
#include <mysql.h>
#pragma comment(lib,"libmysql.lib")

//windows库
#include <Windows.h>

char address[16] = "192.168.1.3";
short port = 11451;
char mysqlAddress[16] = "192.168.1.3";
short mysqlPort = 3306;
char mysqlUserName[1024] = "";
char mysqlPassWord[1024] = "";

//简化namespace
namespace fs = boost::filesystem;
using namespace std;

using pair_void = pair<string, Json::Value(*)(Json::Value, SOCKET conn, sockaddr_in addr)>;//声明类型

//创建全局变量
SOCKET s;																			//创建本地套接字
log_system lg;																		//创建log系统
vector<pair<string, Json::Value(*)(Json::Value, SOCKET, sockaddr_in)>> funcs;		//创建全局函数
vector<pair<string, Json::Value(*)(Json::Value, SOCKET, sockaddr_in)>> voidfuncs;	//创建不返回类型函数
threadpool thpool;

void run(SOCKET conn, sockaddr_in addr);

MYSQL* mysql = new MYSQL;
char query[150]; //查询语句

inline int query_func(const char* chr)
{
	if (mysql_query(mysql, chr))
	{
		printf("Couldn't get result from %s\n", mysql_error(mysql));
		return -1;
	}
	MYSQL_RES* res;
	if (!(res = mysql_store_result(mysql)))    //获得sql语句结束后返回的结果集  
	{
		const char* err = mysql_error(mysql);
		if (strlen(err) > 0)
		{
			printf("Couldn't get result from %s\n", err);
			return -1;
		}
		return 0;
	}
	MYSQL_FIELD* fd;
	while (fd = mysql_fetch_field(res)) {
		cout << fd->name << "\t";
	}
	cout << endl;
	char** sql_row;
	unsigned int col_num = mysql_num_fields(res);
	while (sql_row = mysql_fetch_row(res)) {
		for (int i = 0; i < col_num; i++) {
			if (sql_row[i] == NULL) cout << "NULL\t";
			else cout << sql_row[i] << "\t";
		}
		cout << endl;
	}
	if (res != nullptr) mysql_free_result(res);
	return 0;
}

inline int change_func(const char* chr)
{
	if (mysql_query(mysql, chr))
	{
		printf("Couldn't get result from %s\n", mysql_error(mysql));
		return -1;
	}
	return 0;
}

inline void inConfigFile()
{
	ini::ifile configFile("./config/config.ini");
	configFile.write("mysql", "address", "127.0.0.1");
	configFile.write("mysql", "port", 3306);
	configFile.write("mysql", "username", "");
	configFile.write("mysql", "password", "");
	configFile.write("server", "address", "127.0.0.1");
	configFile.write("server", "port", 11451);
	lg.write("Please fill in the data! in ./config/config.ini");
	exit(-1);
}

inline int init()//加载必须函数
{
	fs::create_directory("./logs");
	fs::create_directory("./config");
	lg = log_system();
	lg.write("log system loaded");

	if (!fs::exists("./config/config.ini"))
	{
		inConfigFile();
	}
	else
	{
		ini::ifile configFile("./config/config.ini");
		bool loadSuccess = true;
		if (strlen(configFile.getCStr("mysql", "address")) < 16)
		{
			strcpy(mysqlAddress, configFile.getCStr("mysql", "address"));
		}
		else loadSuccess = false;
		if (configFile.getInt("mysql", "port") <= 65535 && configFile.getInt("mysql", "port") >= 0)
		{
			mysqlPort = configFile.getInt("mysql", "port");
		}
		else loadSuccess = false;
		if (strlen(configFile.getCStr("mysql", "username")) != 0)
		{
			strcpy(mysqlUserName, configFile.getCStr("mysql", "username"));
		}
		else loadSuccess = false;
		if (strlen(configFile.getCStr("mysql", "password")) != 0)
		{
			strcpy(mysqlPassWord, configFile.getCStr("mysql", "password"));
		}
		else loadSuccess = false;

		if (strlen(configFile.getCStr("server", "address")) < 16)
		{
			strcpy(address, configFile.getCStr("server", "address"));
		}
		else loadSuccess = false;
		if (configFile.getInt("server", "port") <= 65535 && configFile.getInt("mysql", "port") >= 0)
		{
			port = configFile.getInt("server", "port");
		}
		else loadSuccess = false;
		//cout << address << endl << port << endl << mysqlAddress << endl << mysqlPort << endl << mysqlUserName << endl << mysqlPassWord << endl;
		if (!loadSuccess) inConfigFile();
	}
	lg.write("config.ini loaded successfully");

	lg.write("Loading server function...");
	local_class_init();
	lg.write("Loading server function succeeded");

	lg.write("Loading mysql database...");
	mysql_init(mysql);
	if (!mysql_real_connect(mysql, mysqlAddress, mysqlUserName, mysqlPassWord, "mysql", mysqlPort, NULL, NULL))
	{
		lg.write("Failed to load database");
		exit(-1);
	}
	change_func("create database if not exists game default charset GB18030");
	change_func("use game");
	change_func("create table if not exists user (id bigint comment 'id',name varchar(50) comment  'name',password varchar(50) comment  '密码',level int comment 'level',score int comment  'score',socket_win_game int comment '多人游戏赢的局数',ai_win_game int comment  'ai对战中赢的局数',reg_time datetime comment '注册时间',login_time datetime comment '上次登录时间') comment '用户表';");
	lg.write("Database loaded successfully");

	return 0;
}

inline int initsocket()//加载必须的网络模块函数
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		lg.write("wsadata did not load successfully");
		return -1;
	}
	lg.write("creating socket...");
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;                          //模式
	serv_addr.sin_addr.s_addr = inet_addr(address);			 //ip
	serv_addr.sin_port = htons(port);						 //端口

	lg.write("socket created successfully");

	if (_WINSOCK2API_::bind(s, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		lg.write(vstype({ "Binding failed, error code:" ,to_string(WSAGetLastError()) }));
		return -1;
	}
	lg.write(vstype({ "listen address:",inet_ntoa(serv_addr.sin_addr),":",to_string(ntohs(int(serv_addr.sin_port))) }));
	if (_WINSOCK2API_::listen(s, 1024) == SOCKET_ERROR)
	{
		lg.write(vstype({ "Listening failed, error code:" ,to_string(WSAGetLastError()) }));
		return -1;
	}
	else
	{
		lg.write("start listening");
	}

	lg.write("Starting socket receive part...");
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	int len = sizeof(client_addr);
	SOCKET conn;

	lg.write("The socket receiving part starts successfully");
	lg.write("All loading is complete");
	while (true)
	{
		conn = accept(s, (sockaddr*)&client_addr, &len);
		thpool.commit(run, conn, client_addr);
	}
	//thpool.join();
	return 0;
}

int main(int argc, char* argv[])
{
	init();
	initsocket();
	Sleep(5000);
	WSACleanup();
	return 0;
}

inline string deln(const Json::Value& v)
{
	char str[1024] = { 0 };
	strcpy(str, v.toStyledString().c_str());
	string str2;
	for (size_t i = 0; i < strlen(str); i++)
	{
		if (str[i] != '\t' && str[i] != ' ' && str[i] != '\n')
		{
			str2 += str[i];
		}
	}
	return str2;
}

inline void send_msg(const string& json_head, const Json::Value& json_msg, const SOCKET conn, const sockaddr_in addr)
{
	//简单处理返回函数
	//创建变量
	Json::Value er;
	char* strss;
	string sstr;

	er[json_head] = json_msg;//json赋值
	sstr = deln(er);
	strss = new char[sstr.size() + 8 + 1]{0};
	size_t len = headmsg::makeHeadCStr(sstr.c_str(), sstr.size(), strss, sstr.size() + 8 + 1);
	send(conn, strss, len, 0);			//发送
	if (sstr.size()>50)
		lg.write(vstype({ "server>>", inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), ">>", sstr.substr(0,50), "..." }));//打印信息
	else
		lg.write(vstype({ "server>>", inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), ">>", sstr }));

	delete[] strss;
}

inline Json::Value process(char* msg, SOCKET conn, sockaddr_in addr)//处理数据
{
	Json::Value value, re_value;
	Json::Reader reader;
	bool is_research = false;
	map<string, future<Json::Value>> re_map;
	if (!reader.parse(msg, value))//将char*msg变成json对象
	{
		re_value["error"] = "Can't parse json string";
		return re_value;
	}

	Json::Value::Members mem = value.getMemberNames();

	for (auto j = mem.begin(); j != mem.end(); j++)
	{
		is_research = false;
		for (auto i = funcs.begin(); i != funcs.end(); i++)
		{
			if (i->first == *j)
			{
				re_map[i->first] = thpool.commit(i->second, value[i->first], conn, addr);
				is_research = true;
				break;
			}
		}
		if (!is_research)
		{
			for (auto i = voidfuncs.begin(); i != voidfuncs.end(); i++)
			{
				if (i->first == *j)
				{
					thpool.commit(i->second, value[i->first], conn, addr);
					is_research = true;
					break;
				}
			}
		}
		if (!is_research) re_value[*j]["error"] = "Could not find this function";
	}
	for (auto i = re_map.begin(); i != re_map.end(); i++)
		re_value[i->first] = i->second.get();
	return re_value;
}

inline void run(SOCKET conn, sockaddr_in addr)//每个连接处理函数
{
	//创建相应的变量
	headmsg::UnpackHead uh;
	char msg[1025] = { 0 };
	size_t size;
	char* strs;
	Json::Value value;

	lg.write(vstype({ inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), " connected" }));

	try//处理可能发生的问题
	{
		while (true)//只要用户不断开，就会一直处理
		{
			size = recv(conn, msg, sizeof(msg), 0);//处理是否连接断开
			if (size <= 0 || size > sizeof(msg))
			{
				lg.write(vstype({ inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), " disconnected" }));
				break;
			}

			if (size <= 8)
			{
				send_msg("error", "你发的消息不符合规范", conn, addr);//返回问题函数
				break;
			}

			try
			{
				uh = headmsg::UnpackHead(msg, size);//处理消息头
			}
			catch (...)
			{
				send_msg("error", "你发的消息不符合规范", conn, addr);//返回问题函数
				break;
			}

			if (!uh.canGet())
			{
				bool isBreak = false;
				do
				{
					size = recv(conn, msg, sizeof(msg), 0);
					if (size <= 0 || size > sizeof(msg))
					{
						lg.write(vstype({ inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), " disconnected" }));
						isBreak = true;
						break;
					}
					uh.addCStr(msg, size);
				} while (!uh.canGet());
				if (isBreak) break;
			}

			do
			{
				strs = new char[uh.getUnpackedCStrSize() + 1]{ 0 };
				if (!uh.getUnpackedCStr(strs, uh.getUnpackedCStrSize() + 1))
					break;
				if (strlen(strs) <= 50)
					lg.write(vstype({ inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), ">>server>>", strs }));
				else
					lg.write(vstype({ inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), ">>server>>", string(strs).substr(0, 50), "..."}));
				memset(&msg, 0, sizeof(msg));
				strcpy(msg, strs);
				value = process(msg, conn, addr);
				delete[] strs;

				if (value.size() != 0)
				{
					string str = Json::FastWriter().write(value);
					str = str.substr(0, str.length() - 1);
					strs = new char[str.size() + 8 + 1]{ 0 };
					size_t len = headmsg::makeHeadCStr(str.c_str(), str.size(), strs, str.size() + 8 + 1);
					send(conn, strs, len, 0);
					delete[] strs;
					if (str.size() > 50)
						lg.write(vstype({ "server>>", inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), ">>", str.substr(0,50), "..." }));
					else
						lg.write(vstype({ "server>>", inet_ntoa(addr.sin_addr), ":", to_string(short(ntohs(addr.sin_port))).c_str(), ">>", str }));
				}
				value.clear();

			} while (uh.canGet());

			memset(&msg, 0, sizeof(msg));
		}
	}
	catch (...)
	{
		send_msg("error", "你发送的信息导致了错误", conn, addr);
	}
	
	close_socket(conn, addr);
	closesocket(conn);
	return;
}

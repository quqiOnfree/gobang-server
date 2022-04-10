#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <mutex>
#include <fstream>
#include <string>
#include <thread>
#include <ctime>
#include <iostream>
#include <vector>

#include "threadpool.hpp"

using vstype = std::vector<std::string>;

class log_system
{
private:
	std::mutex mtx;
	std::fstream file;
	std::string name;
	bool is_open;
	std::threadpool thpool;

public:
	log_system() : thpool(4)//�����̳߳�+����Ĭ���ļ���log��ʱ��.log��
	{
		std::time_t tt = std::time(0);
			std::tm* time_ = std::localtime((const std::time_t*)&tt);
			name = "./logs/" + std::to_string(1900 + time_->tm_year)
				+ "." + std::to_string(time_->tm_mon + 1) + "."
			+ std::to_string(time_->tm_mday) + ".log";
			file.open(name, std::ios_base::out | std::ios_base::app);//�����׷��ģʽ
		if (file) is_open = true;
			else is_open = false;
	}
	log_system(const std::string&);//�����Զ�������log
	~log_system();

	void write(std::vector<std::string> ve)//������ͬ������һ�£�����һ�����ַ�������
		{	//�÷���class.write()
		if (!is_open) return;
		std::string str;
		for (auto i = ve.begin(); i != ve.end(); i++) str += *i;
		
		auto func = [this](std::string str) {
			std::lock_guard<std::mutex> lg(mtx);
			std::time_t tt = std::time(0);
			std::tm* time_ = std::localtime((const std::time_t*)&tt);
			std::string msgs("");
			msgs += '[' +
				(time_->tm_mday >= 10 ? std::to_string(time_->tm_mday) : std::string("0") + std::to_string(time_->tm_mday)) + " "
				+ (time_->tm_hour >= 10 ? std::to_string(time_->tm_hour) : std::string("0") + std::to_string(time_->tm_hour))
				+ ':' +
				(time_->tm_min >= 10 ? std::to_string(time_->tm_min) : std::string("0") + std::to_string(time_->tm_min)) + ':'
				+ (time_->tm_sec >= 10 ? std::to_string(time_->tm_sec) : std::string("0") + std::to_string(time_->tm_sec))
				+ "]";
			msgs += str;
			std::cout << msgs << std::endl;
			file << msgs << std::endl;
			return;
		};
		thpool.commit(func, str);
		}

	void write(std::string str)//�����ַ����Ĵ���
	{
		if (!is_open) return;//����ļ��Ƿ��
		auto func = [this](std::string str) {
			std::lock_guard<std::mutex> lg(mtx);//��ס�߳�
			std::time_t tt = std::time(0);
			std::tm* time_ = std::localtime((const std::time_t*)&tt);//����ʱ��
			std::string msgs("");
			msgs += '[' +
				(time_->tm_mday >= 10 ? std::to_string(time_->tm_mday) : std::string("0") + std::to_string(time_->tm_mday)) + " "
				+ (time_->tm_hour >= 10 ? std::to_string(time_->tm_hour) : std::string("0") + std::to_string(time_->tm_hour))
				+ ':' +
				(time_->tm_min >= 10 ? std::to_string(time_->tm_min) : std::string("0") + std::to_string(time_->tm_min)) + ':'
				+ (time_->tm_sec >= 10 ? std::to_string(time_->tm_sec) : std::string("0") + std::to_string(time_->tm_sec))
				+ "]";						//msg����logǰ������
			msgs += str;
			std::cout << msgs << std::endl;
			file << msgs << std::endl;
			return;
		};
		thpool.commit(func, str);
	}

	log_system& operator =(const log_system& nlg)//�ำֵ�������ò���
	{
		if (this == &nlg) return *this;
		file.close();
		file.open(nlg.name);
		if (!file) is_open = false;
		else is_open = true;
		name = nlg.name;
		return *this;
	}
};
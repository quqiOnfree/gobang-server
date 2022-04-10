#include "headmsg.hpp"

#include <iostream>

using namespace std;

namespace headmsg
{
	UnpackHead::UnpackHead()
	{
		str = new char[1]{ 0 };
		msglen.size = 0;
		nowLen = 0;
	}

	UnpackHead::UnpackHead(const char* firstMsg, size_t size)
	{
		if (size < 8)
		{
			throw bad_head();
		}

		nowLen = 0;

		memcpy_s(&msglen, sizeof(msglen), firstMsg, sizeof(char) * 8);

		str = new char[size + 1]{ 0 };

		memcpy_s(str, (size - 8) * sizeof(char), firstMsg + 8, (size - 8) * sizeof(char));
		nowLen += size - 8;
	}

	UnpackHead::~UnpackHead()
	{
		delete[] str;
	}

	bool UnpackHead::canGet() const
	{
		if (msglen.size <= nowLen)
			return true;
		return false;
	}

	void UnpackHead::addCStr(const char* in, size_t size)
	{
		char* newStr = new char[nowLen + 1]{ 0 };
		strcpy_s(newStr, nowLen + 1, str);

		delete[] str;

		size_t lastlen = nowLen;
		nowLen += size;
		str = new char[nowLen + 1]{ 0 };
		strcpy_s(str, nowLen + 1, newStr);
		memcpy_s(str + lastlen, (lastlen + size) * sizeof(char), in, size * sizeof(char));
		delete[] newStr;
	}

	size_t UnpackHead::getUnpackedCStr(char* out, size_t outMaxLen)
	{
		if (!this->canGet())
		{
			out[0] = 0;
			return 0;
		}

		memcpy_s(out, msglen.size * sizeof(char), str, std::min(msglen.size * sizeof(char), outMaxLen * sizeof(char)));

		size_t size = msglen.size;

		if (nowLen - msglen.size >= 8)
		{
			char* newStr = new char[nowLen - msglen.size + 1]{ 0 };
			memcpy_s(newStr, (nowLen - msglen.size + 1) * sizeof(char), str + msglen.size, (nowLen - msglen.size) * sizeof(char));
			size_t len = nowLen - msglen.size;
			*this = UnpackHead(newStr, len);
			delete[] newStr;
		}
		else
		{
			*this = UnpackHead();
		}
		return size;
	}

	size_t UnpackHead::getUnpackedCStrSize() const
	{
		return msglen.size;
	}

	UnpackHead& UnpackHead::operator =(const UnpackHead& un)
	{
		if (this == &un)
		{
			return *this;
		}
		memcpy_s(&msglen, sizeof(msglen), &un.msglen, sizeof(un.msglen));
		nowLen = un.nowLen;
		delete[] str;
		str = new char[nowLen + 1]{ 0 };
		memcpy_s(str, sizeof(char) * nowLen, un.str, sizeof(char) * nowLen);
		return *this;
	}

	size_t makeHeadCStr(const char* in, size_t inLen, char* out, size_t outMaxLen)
	{
		MsgLen msglen{ inLen };
		char head[9]{ 0 };
		memcpy(head, &msglen, sizeof(msglen));
		memcpy_s(out, outMaxLen * sizeof(char), &msglen, sizeof(msglen));
		memcpy_s(out + 8, outMaxLen * sizeof(char), in, sizeof(char) * inLen);
		out[inLen + 8] = 0;
		return inLen + 8;
	}
}
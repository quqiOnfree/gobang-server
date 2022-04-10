#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <exception>
#include <cstring>

namespace headmsg
{
	class bad_head : public std::exception
	{
	public:
		const char* what() const throw()
		{
			return "can't unpack this msg!";
		}
	};

	struct MsgLen
	{
		size_t size;
	};

	class UnpackHead
	{
	public:
		UnpackHead();
		UnpackHead(const char* firstMsg, size_t size);
		~UnpackHead();

		bool canGet() const;
		void addCStr(const char* in, size_t size);
		size_t getUnpackedCStr(char* out, size_t outMaxLen);
		size_t getUnpackedCStrSize() const;

		UnpackHead& operator =(const UnpackHead& un);

	private:
		MsgLen msglen;
		size_t nowLen;
		char* str;
	};

	size_t makeHeadCStr(const char* in, size_t inLen, char* out, size_t outMaxLen);
}

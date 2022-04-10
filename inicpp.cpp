#include "inicpp.hpp"

#include <Windows.h>
#include <iostream>
#include <cstring>
#include <fstream>

#include "boost/filesystem.hpp"

using namespace std;
namespace fs = boost::filesystem;

LPCWSTR stwc(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t* wcstring = (wchar_t*)malloc(sizeof(wchar_t) * (orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}

LPCWSTR ctwc(const char* orig_)
{
	string orig(orig_);
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t* wcstring = (wchar_t*)malloc(sizeof(wchar_t) * (orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}

namespace ini
{
	ifile::ifile():filename("")
	{

	}
	
	ifile::ifile(const char* file_name)
	{
		filename = file_name;
	}

	ifile::ifile(const ifile& ifi)
	{
		filename = ifi.filename;
		ifile::ifile(filename.c_str());
	}

	bool ifile::write(const char* appname, const char* keyname, const char* str)
	{
		return WritePrivateProfileString(appname, keyname, str, filename.c_str());
	}

	bool ifile::write(const char* appname, const char* keyname, const int value)
	{
		return WritePrivateProfileString(appname, keyname, to_string(value).c_str(), filename.c_str());
	}

	bool ifile::write(const std::string& appname, const std::string& keyname, const std::string& str)
	{
		return WritePrivateProfileString(appname.c_str(), keyname.c_str(), str.c_str(), filename.c_str());
	}

	bool ifile::write(const std::string& appname, const std::string& keyname, const int value)
	{
		return WritePrivateProfileString(appname.c_str(), keyname.c_str(), to_string(value).c_str(), filename.c_str());
	}

	const char* ifile::getCStr(const char* appname, const char* keyname)
	{
		char strs[10240] = { 0 };
		GetPrivateProfileString(appname, keyname, "", strs, sizeof(strs), filename.c_str());
		return strs;
	}

	const char* ifile::getCStr(const std::string& appname, const std::string& keyname)
	{
		char strs[10240] = { 0 };
		GetPrivateProfileString(appname.c_str(), keyname.c_str(), "", strs, sizeof(strs), filename.c_str());
		return strs;
	}

	long double ifile::getInt(const char* appname, const char* keyname)
	{
		char strs[10240] = { 0 };
		GetPrivateProfileString(appname, keyname, "", strs, sizeof(strs), filename.c_str());
		return static_cast<long double>(atof(strs));
	}

	long double ifile::getInt(const std::string& appname, const std::string& keyname)
	{
		char strs[10240] = { 0 };
		GetPrivateProfileString(appname.c_str(), keyname.c_str(), "", strs, sizeof(strs), filename.c_str());
		return static_cast<long double>(atof(strs));
	}

	long double ifile::getDouble(const std::string& appname, const std::string& keyname)
	{
		char strs[10240] = { 0 };
		GetPrivateProfileString(appname.c_str(), keyname.c_str(), "", strs, sizeof(strs), filename.c_str());
		return static_cast<long double>(atof(strs));
	}

	void ifile::getSection(vector<string>& sec)
	{
		char* section;
		if (fs::exists(filename))
		{
			section = new char[static_cast<long long>(fs::file_size(filename))]{ 0 };
		}
		else
		{
			return;
		}
		long long sec_size = 0;
		char section_name[1024]{ 0 };
		ifstream file;
		file.open(filename, ios_base::in);
		while (!file.eof())
		{
			file >> section_name;
			if (section_name[0] == '[')
			{
				for (long long i = 1; i < strlen(section_name) - 1; i++)
				{
					section[sec_size] = section_name[i];
					sec_size++;
				}
				section[sec_size] = '\0';
				sec_size++;
			}
			memset(&section_name, 0, sizeof(section_name));
		}
		sec_size = 0;
		for (int i = 0; i< static_cast<long long>(fs::file_size(filename)); i++)
		{
			if (!section[i])
			{
				if (!section[i + 1])
				{
					sec.push_back(string(section_name));
					memset(&section_name, 0, sizeof(section_name));
					delete[] section;
					return;
				}
				else
				{
					sec_size = 0;
					sec.push_back(string(section_name));
					memset(&section_name, 0, sizeof(section_name));
				}
			}
			else
			{
				section_name[sec_size] = section[i];
				sec_size++;
			}
		}
		delete[] section;
		return;
	}

	std::string& ifile::getString(const std::string& appname, const std::string& keyname)
	{
		char strs[10240] = { 0 };
		GetPrivateProfileString(appname.c_str(), keyname.c_str(), "", strs, sizeof(strs), filename.c_str());
		string strss = strs;
		return strss;
	}

	bool ifile::deleteSection(const char* name)
	{
		return WritePrivateProfileString(name, NULL, NULL, filename.c_str());
	}

	bool ifile::deleteSection(const std::string& name)
	{
		return WritePrivateProfileString(name.c_str(), NULL, NULL, filename.c_str());
	}

	bool ifile::deleteKey(const char* section, const char* keyname)
	{
		return WritePrivateProfileString(section, keyname, NULL, filename.c_str());
	}

	bool ifile::deleteKey(const std::string& section, const std::string& keyname)
	{
		return WritePrivateProfileString(section.c_str(), keyname.c_str(), NULL, filename.c_str());
	}

	ifile& ifile::operator=(const ifile& i)
	{
		if (this == &i)
		{
			return *this;
		}

		filename = i.filename;
		ifile::ifile(filename.c_str());
		return *this;
	}

	ifile::~ifile()
	{
		
	}
}
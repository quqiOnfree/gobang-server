#include "log_system.hpp"
#include <ctime>

using namespace std;

log_system::log_system(const string& str) : thpool(4)
{
	name = str;
	file.open(str, ios_base::out | ios_base::app);
	if (!file) { is_open = false; }
	else { is_open = true; }
}

log_system::~log_system()
{
	file.close();
}



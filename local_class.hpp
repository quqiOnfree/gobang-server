#pragma once

#ifndef LOCAL_CLASS
#define LOCAL_CLASS

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//�ڴ��м����뺯��������
void local_class_init();
void close_socket(SOCKET sock, sockaddr_in addr);
#endif // !LOCAL_CLASS

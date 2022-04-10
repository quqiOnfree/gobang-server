#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/des.h>

#ifdef __cplusplus //如果是c\py用户请把extern "C"的注释去掉

#include <string>
#define XEXT extern "C"

#else
#define XEXT
#endif

#ifdef _WIN32
#define XLIB XEXT __declspec(dllexport)
#else
#define XLIB XEXT
#endif

//公钥
#define PUB_KEY "pubkey.pem"
//私钥
#define PRI_KEY "prikey.pem"

/// <summary>
/// 打印大整数
/// </summary>
/// <param name="n"></param>
XLIB void printBn(const BIGNUM* n);

/// <summary>
/// 打印二进制unsigned char*
/// </summary>
/// <param name="str">字符串</param>
/// <param name="len">字符串长度</param>
XLIB void printBytes(unsigned char* str, size_t len);

/// <summary>
/// base64加密
/// </summary>
/// <param name="in">需要加密的字符串</param>
/// <param name="len">需要加密的字符串的长度</param>
/// <param name="out_base64">输出的字符串</param>
/// <returns>返回输出字符串的长度</returns>
XLIB int base64Encode(const unsigned char* in, int len, char* out_base64);

/// <summary>
/// base64解密
/// </summary>
/// <param name="in">输入的字符串</param>
/// <param name="len">输入的字符串长度</param>
/// <param name="out_data">输出的字符串</param>
/// <returns>输出的字符串的长度</returns>
XLIB int base64Decode(const char* in, int len, unsigned char* out_data);

/// <summary>
/// 生成rsa密钥参数 返回的pkey需要调用者释放
/// 函数：EVP_PKEY_free()
/// </summary>
/// <param name="size">密钥大小</param>
/// <returns>返回EVP_PKEY*</returns>
XLIB EVP_PKEY* getRsaKey(const int size);

/// <summary>
/// rsa加密
/// </summary>
/// <param name="in">输入的二进制字符串</param>
/// <param name="len">输入的二进制字符串长度</param>
/// <param name="out">输出的二进制字符串</param>
/// <returns>输出的二进制字符串的长度</returns>
XLIB int rsaEncrypt(const unsigned char* in, int len, unsigned char* out);

/// <summary>
/// rsa解密
/// </summary>
/// <param name="in">输入的二进制字符串</param>
/// <param name="len">输入的二进制字符串长度</param>
/// <param name="out">输出的二进制字符串</param>
/// <returns>输出的二进制字符串长度</returns>
XLIB int rsaDecrypt(const unsigned char* in, int len, unsigned char* out);

/// <summary>
/// 写入公钥私钥
/// </summary>
/// <param name="gk">evp_pkey*</param>
/// <returns>没有返回</returns>
XLIB void writeRsaKey(const EVP_PKEY* gk);

/// <summary>
/// des加密 加解密文件
/// </summary>
/// <param name="passwd">密钥</param>
/// <param name="in_filename">输入的文件名</param>
/// <param name="out_filename">输出的文件名</param>
/// <param name="is_enc">加密还是解密（加密true）</param>
/// <returns></returns>
bool Encrypt_File(std::string passwd, std::string in_filename, std::string out_filename, bool is_enc);

/// <summary>
/// XSecEncryptFile 加密 加解密文件
/// </summary>
/// <param name="passwd">密钥</param>
/// <param name="in_filename">输入的文件名</param>
/// <param name="out_filename">输出的文件名</param>
/// <param name="is_enc">加密还是解密（加密true）</param>
/// <returns></returns>
bool XSecEncryptFile(std::string passwd, std::string in_filename, std::string out_filename, bool is_enc);

XLIB int aesEncrypt(const int type, const unsigned char* in, const int in_size, const char* key, unsigned char* out);
XLIB int aesDecrypt(const int type, const unsigned char* in, const int in_size, const char* key, unsigned char* out);

enum XsecType
{
	XDES_ECB,
	XDES_CBC,
	X3DES_ECB,
	X3DES_CBC,
	XAES128_ECB,
	XAES128_CBC,
	XAES192_ECB,
	XAES192_CBC,
	XAES256_ECB,
	XAES256_CBC,
};

class XSec
{
private:
	/// <summary>
	/// DES ECB 模式加密
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int EnDesECB(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES ECB 模式解密
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int DeDesECB(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES CBC 模式加密
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int EnDesCBC(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES CBC 模式解密
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int DeDesCBC(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES算法密钥
	/// </summary>
	DES_key_schedule ks_;

	/// <summary>
	/// 加密算法类型
	/// </summary>
	XsecType type_;

	/// <summary>
	/// 是否加密
	/// </summary>
	bool is_en_ = false;

	/// <summary>
	/// 数据块大小，分组大小
	/// </summary>
	int block_size_ = 0;

	/// <summary>
	/// 初始化向量
	/// </summary>
	unsigned char iv_[128] = { 0 };

	/// <summary>
	/// 加解密ctx上下文
	/// </summary>
	void* ctx_ = 0;
public:
	/// <summary>
	/// 初始化加密对象，清理之前的数据
	/// </summary>
	/// <param name="type">加密类型</param>
	/// <param name="pass">密钥，可以是二进制</param>
	/// <param name="is_en">加密，否则解密</param>
	/// <returns>是否成功</returns>
	virtual bool Init(XsecType type, const std::string& pass, bool is_en);

	/// <summary>
	/// 加解密数据
	/// </summary>
	/// <param name="in">输入数据</param>
	/// <param name="in_size">数据大小</param>
	/// <param name="out">输出数据</param>
	/// <returns>成功返回加解密后数据字节大小，失败后返回0</returns>
	virtual int Encrypt(const unsigned char* in, int in_size, unsigned char* out, bool is_end = true);

	/// <summary>
	/// 释放class
	/// </summary>
	virtual void Close();
};

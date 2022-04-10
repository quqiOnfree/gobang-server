#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/des.h>

#ifdef __cplusplus //�����c\py�û����extern "C"��ע��ȥ��

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

//��Կ
#define PUB_KEY "pubkey.pem"
//˽Կ
#define PRI_KEY "prikey.pem"

/// <summary>
/// ��ӡ������
/// </summary>
/// <param name="n"></param>
XLIB void printBn(const BIGNUM* n);

/// <summary>
/// ��ӡ������unsigned char*
/// </summary>
/// <param name="str">�ַ���</param>
/// <param name="len">�ַ�������</param>
XLIB void printBytes(unsigned char* str, size_t len);

/// <summary>
/// base64����
/// </summary>
/// <param name="in">��Ҫ���ܵ��ַ���</param>
/// <param name="len">��Ҫ���ܵ��ַ����ĳ���</param>
/// <param name="out_base64">������ַ���</param>
/// <returns>��������ַ����ĳ���</returns>
XLIB int base64Encode(const unsigned char* in, int len, char* out_base64);

/// <summary>
/// base64����
/// </summary>
/// <param name="in">������ַ���</param>
/// <param name="len">������ַ�������</param>
/// <param name="out_data">������ַ���</param>
/// <returns>������ַ����ĳ���</returns>
XLIB int base64Decode(const char* in, int len, unsigned char* out_data);

/// <summary>
/// ����rsa��Կ���� ���ص�pkey��Ҫ�������ͷ�
/// ������EVP_PKEY_free()
/// </summary>
/// <param name="size">��Կ��С</param>
/// <returns>����EVP_PKEY*</returns>
XLIB EVP_PKEY* getRsaKey(const int size);

/// <summary>
/// rsa����
/// </summary>
/// <param name="in">����Ķ������ַ���</param>
/// <param name="len">����Ķ������ַ�������</param>
/// <param name="out">����Ķ������ַ���</param>
/// <returns>����Ķ������ַ����ĳ���</returns>
XLIB int rsaEncrypt(const unsigned char* in, int len, unsigned char* out);

/// <summary>
/// rsa����
/// </summary>
/// <param name="in">����Ķ������ַ���</param>
/// <param name="len">����Ķ������ַ�������</param>
/// <param name="out">����Ķ������ַ���</param>
/// <returns>����Ķ������ַ�������</returns>
XLIB int rsaDecrypt(const unsigned char* in, int len, unsigned char* out);

/// <summary>
/// д�빫Կ˽Կ
/// </summary>
/// <param name="gk">evp_pkey*</param>
/// <returns>û�з���</returns>
XLIB void writeRsaKey(const EVP_PKEY* gk);

/// <summary>
/// des���� �ӽ����ļ�
/// </summary>
/// <param name="passwd">��Կ</param>
/// <param name="in_filename">������ļ���</param>
/// <param name="out_filename">������ļ���</param>
/// <param name="is_enc">���ܻ��ǽ��ܣ�����true��</param>
/// <returns></returns>
bool Encrypt_File(std::string passwd, std::string in_filename, std::string out_filename, bool is_enc);

/// <summary>
/// XSecEncryptFile ���� �ӽ����ļ�
/// </summary>
/// <param name="passwd">��Կ</param>
/// <param name="in_filename">������ļ���</param>
/// <param name="out_filename">������ļ���</param>
/// <param name="is_enc">���ܻ��ǽ��ܣ�����true��</param>
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
	/// DES ECB ģʽ����
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int EnDesECB(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES ECB ģʽ����
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int DeDesECB(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES CBC ģʽ����
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int EnDesCBC(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES CBC ģʽ����
	/// </summary>
	/// <param name="in"></param>
	/// <param name="in_size"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	int DeDesCBC(const unsigned char* in, int in_size, unsigned char* out);

	/// <summary>
	/// DES�㷨��Կ
	/// </summary>
	DES_key_schedule ks_;

	/// <summary>
	/// �����㷨����
	/// </summary>
	XsecType type_;

	/// <summary>
	/// �Ƿ����
	/// </summary>
	bool is_en_ = false;

	/// <summary>
	/// ���ݿ��С�������С
	/// </summary>
	int block_size_ = 0;

	/// <summary>
	/// ��ʼ������
	/// </summary>
	unsigned char iv_[128] = { 0 };

	/// <summary>
	/// �ӽ���ctx������
	/// </summary>
	void* ctx_ = 0;
public:
	/// <summary>
	/// ��ʼ�����ܶ�������֮ǰ������
	/// </summary>
	/// <param name="type">��������</param>
	/// <param name="pass">��Կ�������Ƕ�����</param>
	/// <param name="is_en">���ܣ��������</param>
	/// <returns>�Ƿ�ɹ�</returns>
	virtual bool Init(XsecType type, const std::string& pass, bool is_en);

	/// <summary>
	/// �ӽ�������
	/// </summary>
	/// <param name="in">��������</param>
	/// <param name="in_size">���ݴ�С</param>
	/// <param name="out">�������</param>
	/// <returns>�ɹ����ؼӽ��ܺ������ֽڴ�С��ʧ�ܺ󷵻�0</returns>
	virtual int Encrypt(const unsigned char* in, int in_size, unsigned char* out, bool is_end = true);

	/// <summary>
	/// �ͷ�class
	/// </summary>
	virtual void Close();
};

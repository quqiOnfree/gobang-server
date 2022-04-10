#include "endecode.hpp"

#include <iostream>
#include <fstream>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#ifdef _WIN32
#include <openssl/applink.c>
#endif

using namespace std;

void printBn(const BIGNUM* n)
{
	unsigned char to[256] = { 0 };
	BN_bn2bin(n, to);
	int byte_size = BN_num_bytes(n);
	for (int i = 0; i < byte_size; i++)
	{
		printf("%02x", to[i]);
	}
	printf("\n");
}

void printBytes(unsigned char* str, size_t len)
{
	cout << "b'";
	for (size_t i = 0; i < len; i++)
	{
		cout << "\\x" << hex << (int)str[i];
	}
	cout << "'" << endl;
}

int base64Encode(const unsigned char* in, int len, char* out_base64)
{
	if (!in || len <= 0 || !out_base64) return 0;
	auto mem_bio = BIO_new(BIO_s_mem());
	if (!mem_bio) return 0;
	auto b64_bio = BIO_new(BIO_f_base64());
	if (!b64_bio)
	{
		BIO_free(mem_bio);
		return 0;
	} 

	BIO_push(b64_bio, mem_bio);
	BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);

	int re = BIO_write(b64_bio, in, len);
	if (re <= 0)
	{
		BIO_free_all(b64_bio);
		return 0;
	}

	int outsize = 0;
	BIO_flush(b64_bio);
	BUF_MEM* p_data = 0;
	BIO_get_mem_ptr(b64_bio, &p_data);
	if (p_data)
	{
		memcpy(out_base64, p_data->data, p_data->length);
		outsize = p_data->length;
	}
	BIO_free_all(b64_bio);
	return outsize;
}

int base64Decode(const char* in, int len, unsigned char* out_data)
{
	if (!in || len < 0 || !out_data) return 0;
	auto mem_bio = BIO_new_mem_buf(in, len);
	if (!mem_bio) return 0;
	auto b64_bio = BIO_new(BIO_f_base64());
	if (!b64_bio)
	{
		BIO_free(mem_bio);
		return 0;
	}

	BIO_push(b64_bio, mem_bio);
	BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);
	size_t size = 0;
	BIO_read_ex(b64_bio, out_data, len, &size);
	BIO_free_all(b64_bio);
	return size;
}

EVP_PKEY* getRsaKey(const int size)
{
	auto ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
	if (!ctx)
	{
		ERR_print_errors_fp(stderr);
		return 0;
	}
	if (EVP_PKEY_keygen_init(ctx) <= 0)
	{
		ERR_print_errors_fp(stderr);
		EVP_PKEY_CTX_free(ctx);
		return 0;
	}
	if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, size) <= 0)
	{
		ERR_print_errors_fp(stderr);
		EVP_PKEY_CTX_free(ctx);
		return 0;
	}

	EVP_PKEY* pkey = NULL;
	if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
	{
		ERR_print_errors_fp(stderr);
		EVP_PKEY_CTX_free(ctx);
		return 0;
	}

	EVP_PKEY_CTX_free(ctx);

	//auto tp = EVP_PKEY_gettable_params(pkey);
	//while (tp)
	//{
	//	if (!tp->key) break;
	//	cout << tp->key << endl;
	//	tp++;
	//}

	return pkey;
}

int rsaEncrypt(const unsigned char* in, int len, unsigned char* out)
{
	FILE* file = fopen(PUB_KEY, "r");
	if (!file) return 0;

	RSA* r = NULL;
	PEM_read_RSAPublicKey(file, &r, NULL, NULL);
	fclose(file);
	if (!r)
	{
		ERR_print_errors_fp(stderr);
		return 0;
	}
	int key_size = RSA_size(r);
	EVP_PKEY* pkey = EVP_PKEY_new();
	EVP_PKEY_set1_RSA(pkey, r);
	auto ctx = EVP_PKEY_CTX_new(pkey, NULL);
	EVP_PKEY_free(pkey);
	RSA_free(r);

	EVP_PKEY_encrypt_init(ctx);
	int block_size = key_size - RSA_PKCS1_PADDING_SIZE;
	int out_size = 0;

	size_t out_len = 0;
	size_t bsize = 0;
	for (int i = 0; i < len; i += block_size)
	{
		out_len = key_size;
		bsize = block_size;

		if (len - i < block_size) bsize = len - i;

		if (EVP_PKEY_encrypt(ctx, out + out_size, &out_len, in + i, bsize) <= 0)
		{
			ERR_print_errors_fp(stderr);
			break;
		}
		out_size += out_len;
	}
	
	EVP_PKEY_CTX_free(ctx);
	return out_size;
}

int rsaDecrypt(const unsigned char* in, int len, unsigned char* out)
{
	int out_size = 0;
	FILE* file = fopen(PRI_KEY, "r");
	if (!file) return 0;
	RSA* r = RSA_new();
	PEM_read_RSAPrivateKey(file, &r, NULL, NULL);
	fclose(file);
	if (!r)
	{
		ERR_print_errors_fp(stderr);
		return 0;
	}

	int key_size = RSA_size(r);
	EVP_PKEY* pkey = EVP_PKEY_new();
	EVP_PKEY_set1_RSA(pkey, r);
	auto ctx = EVP_PKEY_CTX_new(pkey, NULL);
	EVP_PKEY_free(pkey);
	RSA_free(r);

	EVP_PKEY_decrypt_init(ctx);

	size_t out_len = 0;
	for (int i = 0; i < len; i++)
	{
		out_len = key_size;
		if (EVP_PKEY_decrypt(ctx, out + out_size, &out_len, in + i, key_size) <= 0)
		{
			ERR_print_errors_fp(stderr);
			break;
		}
		out_size += out_len;
	}

	EVP_PKEY_CTX_free(ctx);
	return out_size;
}

void writeRsaKey(const EVP_PKEY* gk)
{
	FILE* file = fopen(PUB_KEY, "w");
	PEM_write_RSAPublicKey(file, EVP_PKEY_get0_RSA(gk));
	fclose(file);

	file = fopen(PRI_KEY, "w");
	PEM_write_RSAPrivateKey(file, EVP_PKEY_get0_RSA(gk), NULL, NULL, 0, NULL, NULL);
	fclose(file);
}

bool Encrypt_File(string passwd, string in_filename, string out_filename, bool is_enc)
{
	//选择加解密算法，后面可以替换
	auto cipher = EVP_des_ede3_cbc();

	//输入文件大小
	int in_file_size = 0;

	//输出文件大小
	int out_file_size = 0;
	ifstream ifs(in_filename, ios::binary); //二进制打开输入文件
	if (!ifs)return false;
	ofstream ofs(out_filename, ios::binary);//二进制大小输出文件
	if (!ofs)
	{
		ifs.close();
		return false;
	}
	auto ctx = EVP_CIPHER_CTX_new(); //加解密上下文

	//密钥初始化 多出的丢弃
	unsigned char key[128] = { 0 };
	int key_size = EVP_CIPHER_key_length(cipher);// 获取密钥长度
	if (key_size > passwd.size())   //密码少了
	{
		key_size = passwd.size();
	}
	memcpy(key, passwd.data(), key_size);

	unsigned char iv[128] = { 0 }; //初始化向量
	int re = EVP_CipherInit(ctx, cipher, key, iv, is_enc);
	if (!re)
	{
		ERR_print_errors_fp(stderr);
		ifs.close();
		ofs.close();
		EVP_CIPHER_CTX_free(ctx);
		return false;
	}
	unsigned char buf[1024] = { 0 };
	unsigned char out[1024] = { 0 };
	int out_len = 0;
	//1 读文件=》2 加解密文件=》3写入文件
	while (!ifs.eof())
	{
		//1 读文件
		ifs.read((char*)buf, sizeof(buf));
		int count = ifs.gcount();
		if (count <= 0)break;
		in_file_size += count; //统计读取文件大小
		//2 加解密文件 机密到out
		EVP_CipherUpdate(ctx, out, &out_len, buf, count);
		if (out_len <= 0)break;
		//3 写入文件
		ofs.write((char*)out, out_len);
		out_file_size += out_len;
	}
	//取出最后一块数据
	EVP_CipherFinal(ctx, out, &out_len);
	if (out_len > 0)
	{
		ofs.write((char*)out, out_len);
		out_file_size += out_len;
	}

	ifs.close();
	ofs.close();
	EVP_CIPHER_CTX_free(ctx);
	cout << "in_file_size:" << in_file_size << endl;
	cout << "out_file_size:" << out_file_size << endl;
	return true;
}

bool XSecEncryptFile(string passwd, string in_filename, string out_filename, bool is_enc)
{
	ifstream ifs(in_filename, ios::binary); //二进制打开输入文件
	if (!ifs)return false;
	ofstream ofs(out_filename, ios::binary);//二进制大小输出文件
	if (!ofs)
	{
		ifs.close();
		return false;
	}

	XSec sec;
	sec.Init(XAES128_CBC, "1234567812345678", is_enc);

	unsigned char buf[1024] = { 0 };
	unsigned char out[1024] = { 0 };
	int out_len = 0;
	//1 读文件=》2 加解密文件=》3写入文件
	int count__ = 0;
	bool is_end = false;
	while (!ifs.eof())
	{
		//1 读文件
		ifs.read((char*)buf, sizeof(buf));
		count__ = ifs.gcount();
		if (count__ <= 0)break;
		is_end = false;
		if (ifs.eof()) is_end = true;
		out_len =  sec.Encrypt(buf, count__, out, is_end);
		if (out_len <= 0) break;
		ofs.write((char*)out, out_len);
	}

	sec.Close();
	ifs.close();
	ofs.close();
	return true;
}

int aesEncrypt(const int type, const unsigned char* in, const int in_size, const char* key, unsigned char* out)
{
	XSec sec;
	sec.Init((XsecType)type, key, true);
	return sec.Encrypt(in, in_size, out);
}

int aesDecrypt(const int type, const unsigned char* in, const int in_size, const char* key, unsigned char* out)
{
	XSec sec;
	sec.Init((XsecType)type, key, false);
	return sec.Encrypt(in, in_size, out);
}

//XSec::~XSec()
//{
//	Close();
//}

void XSec::Close()
{
	memset(iv_, 0, sizeof(iv_));

	if (ctx_)
	{
		EVP_CIPHER_CTX_free((EVP_CIPHER_CTX*)ctx_);
		ctx_ = nullptr;
	}
}

bool XSec::Init(XsecType type, const std::string& pass, bool is_en)
{
	Close();

	this->type_ = type;
	this->is_en_ = is_en;

	unsigned char key[32] = {0};
	int key_size = pass.size();

	const EVP_CIPHER* cipher = 0;

	switch (type)
	{
	case XDES_ECB:
	case XDES_CBC:
		block_size_ = DES_KEY_SZ;
		if (key_size > block_size_) key_size = block_size_;
		memcpy(key, pass.data(), key_size);
		DES_set_key((const_DES_cblock*)key, &ks_);
		return true;
	case X3DES_ECB:
		cipher = EVP_des_ede3_ecb();
		break;
	case X3DES_CBC:
		cipher = EVP_des_ede3_cbc();
		break;
	case XAES128_ECB:
		cipher = EVP_aes_128_ecb();
		break;
	case XAES128_CBC:
		cipher = EVP_aes_128_cbc();
		break;
	case XAES192_ECB:
		cipher = EVP_aes_192_ecb();
		break;
	case XAES192_CBC:
		cipher = EVP_aes_192_cbc();
		break;
	case XAES256_ECB:
		cipher = EVP_aes_256_ecb();
		break;
	case XAES256_CBC:
		cipher = EVP_aes_256_cbc();
		break;
	default:
		break;
	}

	if (!cipher)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}

	block_size_ = EVP_CIPHER_block_size(cipher);

	if (key_size > EVP_CIPHER_key_length(cipher))
		key_size = EVP_CIPHER_key_length(cipher);
	memcpy(key, pass.data(), key_size);

	ctx_ = EVP_CIPHER_CTX_new();

	int re = EVP_CipherInit((EVP_CIPHER_CTX*)ctx_, cipher, key, iv_, is_en);
	if (!re)
	{
		ERR_print_errors_fp(stderr);
		return false;
	}
	return true;
}

int XSec::Encrypt(const unsigned char* in, int in_size, unsigned char* out, bool is_end)
{
	if (type_ == XDES_ECB)
	{
		if (is_en_)
		{
			return EnDesECB(in, in_size, out);
		}
		else
		{
			return DeDesECB(in, in_size, out);
		}
	}
	else if (type_ == XDES_CBC)
	{
		if (is_en_)
		{
			return EnDesCBC(in, in_size, out);
		}
		else
		{
			return DeDesCBC(in, in_size, out);
		}
	}
	if (is_end)
	{
		EVP_CIPHER_CTX_set_padding((EVP_CIPHER_CTX*)ctx_, EVP_PADDING_PKCS7);
	}
	else
	{
		EVP_CIPHER_CTX_set_padding((EVP_CIPHER_CTX*)ctx_, 0);
	}

	int out_len = 0;
	EVP_CipherUpdate((EVP_CIPHER_CTX*)ctx_, out, &out_len, in, in_size);
	if (out_len <= 0)
		return 0;
	int out_pandding_len = 0;
	EVP_CipherFinal((EVP_CIPHER_CTX*)ctx_, out + out_len, &out_pandding_len);
	return out_len + out_pandding_len;
}

int XSec::EnDesECB(const unsigned char* in, int in_size, unsigned char* out)
{
	/// 数据填充 PKCS7 Padding
	unsigned char pad[8]{ 0 };
	int padding_size = block_size_ - (in_size % block_size_);
	memset(pad, padding_size, sizeof(pad));

	int i = 0;
	for ( ; i < in_size; i+=block_size_)
	{
		if (in_size - i < block_size_)
		{
			memcpy(pad, in + i, in_size - i);
			break;
		}
		DES_ecb_encrypt((const_DES_cblock*)(in + i), (DES_cblock*)(out + i), &ks_, DES_ENCRYPT);
	}

	//补充 pkcs7结尾
	DES_ecb_encrypt((const_DES_cblock*)pad, (DES_cblock*)(out + i), &ks_, DES_ENCRYPT);

	return in_size + padding_size;
}

int XSec::DeDesECB(const unsigned char* in, int in_size, unsigned char* out)
{
	for (int i = 0; i < in_size; i += block_size_)
	{
		DES_ecb_encrypt((const_DES_cblock*)(in + i), (DES_cblock*)(out + i), &ks_, DES_DECRYPT);
	}
	return in_size - out[in_size - 1];
}

int XSec::EnDesCBC(const unsigned char* in, int in_size, unsigned char* out)
{
	unsigned char pad[8]{ 0 };
	int padding_size = block_size_ - (in_size % block_size_);
	memset(pad, padding_size, sizeof(pad));
	int size1 = in_size - (in_size % block_size_);
	DES_ncbc_encrypt(in, out, size1, &ks_, (DES_cblock*)iv_, DES_ENCRYPT);

	if (in_size % block_size_ != 0)
	{
		memcpy(pad, in + size1, (in_size % block_size_));
	}
	DES_ncbc_encrypt(pad, out + size1, sizeof(pad), &ks_, (DES_cblock*)iv_, DES_ENCRYPT);
	return in_size + padding_size;
}

int XSec::DeDesCBC(const unsigned char* in, int in_size, unsigned char* out)
{
	DES_ncbc_encrypt(in, out, in_size, &ks_, (DES_cblock*)iv_, DES_DECRYPT);
	return in_size - out[in_size - 1];
}
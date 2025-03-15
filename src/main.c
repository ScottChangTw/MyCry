#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>

#include <sodium.h>

#include "file_info.h"
#include "simple_png.h"
#include "simple_crc.h"

#define CHUNK_SIZE 4096

int encrypt_file(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE];
    unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
    fwrite(header, 1, sizeof header, fp_t);
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
        crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, rlen,
                                                   NULL, 0, tag);
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);
    fclose(fp_t);
    fclose(fp_s);
    return 0;
}

int decrypt_file(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  buf_out[CHUNK_SIZE];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    int            ret = -1;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    fread(header, 1, sizeof header, fp_s);
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
        goto ret; /* incomplete header */
    }
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
                                                       buf_in, rlen, NULL, 0) != 0) {
            goto ret; /* corrupted chunk */
        }
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
            if (! eof) {
                goto ret; /* end of stream reached before the end of the file */
            }
        } else { /* not the final chunk yet */
            if (eof) {
                goto ret; /* end of file reached before the end of the stream */
            }
        }
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);

    ret = 0;
ret:
    fclose(fp_t);
    fclose(fp_s);
    return ret;
}

static int encrypt_buf(unsigned char **crypt_out, const unsigned char *message_in, unsigned long long message_len, const char *nonce_str, const char *key_str)
{
	unsigned char *crypt_buf;
	char *nonce;
	char *key;
	int result;

	crypt_buf = (unsigned char *)malloc(crypto_secretbox_xchacha20poly1305_MACBYTES + message_len);

	nonce = sodium_malloc(crypto_secretbox_xchacha20poly1305_NONCEBYTES);
	sodium_memzero(nonce, crypto_secretbox_xchacha20poly1305_NONCEBYTES);

	strncpy(nonce, nonce_str, crypto_secretbox_xchacha20poly1305_NONCEBYTES);

	key = sodium_malloc(crypto_secretbox_xchacha20poly1305_KEYBYTES);
	sodium_memzero(key, crypto_secretbox_xchacha20poly1305_KEYBYTES);
	
	strncpy(key, key_str, crypto_secretbox_xchacha20poly1305_KEYBYTES);

	result = crypto_secretbox_xchacha20poly1305_easy(crypt_buf, message_in, message_len, (unsigned char *)nonce, (unsigned char *)key);

	if(result < 0)  
		printf("%s error !!!!\n", __func__);

	*crypt_out = crypt_buf;
	
	return crypto_secretbox_xchacha20poly1305_MACBYTES + message_len;
}

static int encrypt_file_pad(const char *target_file, const char *source_file)
{
	unsigned char *inBuf;
	uint32_t inf_crc;
	unsigned char *outBuf;
	unsigned char *outBuf_pad;

	unsigned int inFileSize =  0;
	unsigned int outSize =  0;
	char *inFileName;
	size_t outSize_pad =  0;
	char key[32];

	inBuf = read_file_to_string(source_file, &inFileSize);

	inf_crc = Simple_CRC_SlicingBy8(0, inBuf, inFileSize);

	printf("input file CRC = 0x%08X\n", inf_crc);

	printf("input key:");
	fgets((char *)key, sizeof(key), stdin);

	outSize = encrypt_buf(&outBuf, inBuf, inFileSize, "1", key);

	outBuf_pad = realloc(outBuf, outSize + 4096);

	sodium_pad(&outSize_pad, outBuf_pad, outSize, 4096, outSize + 4096);

	printf("encrypt size :%d, pad to %zu\n", outSize, outSize_pad);
	
	//dump_mem(outBuf_pad, outSize_pad, "%s", target_file);

	//simple_write_argb_to_fpng(outBuf_pad, 1024, outSize_pad/4096, "%s", target_file);
	inFileName = get_file_name(source_file);
	printf("set text %s to png\n", inFileName);
	simple_set_text_libpng("%s", inFileName);
	simple_write_argb_to_libpng(outBuf_pad, 1024, outSize_pad/4096, "%s", target_file);
	
	printf("PNG size : 1024x%zu\n", outSize_pad/4096);

	return 0;
}

static int decrypt_buf(unsigned char **decrypted, const unsigned char *cipher_buf, unsigned long long cipher_len, const char *nonce_str, const char *key_str)
{
	unsigned char *decrypted_buf;
	char *nonce;
	char *key;
	int result;

	decrypted_buf = (unsigned char *)malloc(crypto_secretbox_xchacha20poly1305_MACBYTES + cipher_len);

	nonce = sodium_malloc(crypto_secretbox_xchacha20poly1305_NONCEBYTES);
	sodium_memzero(nonce, crypto_secretbox_xchacha20poly1305_NONCEBYTES);

	strncpy(nonce, nonce_str, crypto_secretbox_xchacha20poly1305_NONCEBYTES);

	key = sodium_malloc(crypto_secretbox_xchacha20poly1305_KEYBYTES);
	sodium_memzero(key, crypto_secretbox_xchacha20poly1305_KEYBYTES);
	
	strncpy(key, key_str, crypto_secretbox_xchacha20poly1305_KEYBYTES);

	result = crypto_secretbox_xchacha20poly1305_open_easy(decrypted_buf, cipher_buf, cipher_len, (unsigned char *)nonce, (unsigned char *)key);
	if (result !=0)
	{
		printf("sodium open %d\n", result);
		return -1;
	}

	*decrypted = decrypted_buf;

	return cipher_len - crypto_secretbox_xchacha20poly1305_MACBYTES;

}
static int decrypt_file_pad(const char *target_file, const char *source_file)
{
	unsigned char *inBuf;
	unsigned char *outBuf = NULL;
	unsigned int inFileSize =  0;
	unsigned int outFileSize =  0;
	char key[32];
	size_t unpadded_buflen;
	uint32_t w,h;
	char *title = NULL;

	//inBuf = read_file_to_string(source_file, &inFileSize);

	//simple_read_argb_from_fpng(source_file, &inBuf, &w, &h, NULL);
	simple_read_argb_from_libpng(source_file, &inBuf, &w, &h, &title);
	inFileSize = w * h * 4;
	
	printf("PNG WxH : %dx%d, size = %d\n", w, h, inFileSize);

	sodium_unpad(&unpadded_buflen, inBuf, inFileSize, 4096);

	printf("unpadded_buflen size = %zu\n", unpadded_buflen);

	printf("input key:");
	fgets((char *)key, sizeof(key), stdin);

	if((outFileSize = decrypt_buf(&outBuf, inBuf, unpadded_buflen, "1", key)) >= 0)
	{
		uint32_t outf_crc;
		
		printf("size :%zu, decrypt to %d\n", unpadded_buflen, outFileSize);
		
		outf_crc = Simple_CRC_SlicingBy8(0, outBuf, outFileSize);
		
		printf("output file CRC = 0x%08X\n", outf_crc);
		
		dump_mem(outBuf, outFileSize, "%s", title == NULL ? target_file : title);
	}

	if(inBuf)   free(inBuf);
	if(outBuf)  free(outBuf);
	
	return 0;
}

int main(int argc, char *argv[])
{
//	unsigned char *key;

	char *cInfile = NULL;
	char *cInfile_wo_ext = NULL;
	char *cOutfile = NULL;

	int Op = 0; // 0: None, 1:Enc, 2: Dec
	int result;

	printf("start~\n");

	if (sodium_init() != 0) {
		printf("sodium not init success !!!!\n");	
        return 1;
    }
	

	do
	{
		int opt;
		while ((opt = getopt(argc, argv, "e:d:")) != -1) 
		{
			switch (opt) 
			{
				case 'e':
					cInfile = strdup(optarg);
					printf("set input file = %s\n", cInfile);
					Op = 1;
					break;
				case 'd':
					cInfile = strdup(optarg);
					printf("set input file = %s\n", cInfile);
					Op = 2;
					break;				
				default:
					printf("opt %c unknow\n", opt);
					break;
					
			}

		}
	}while(0);
	
	
#if 0
	switch(Op)
	{
		case 0:
			printf("op error !!!\n");
			result = -1;
			break;
		case 1:
			asprintf(&cOutfile,"%s.enc", cInfile);
		    result = encrypt_file(cOutfile, cInfile, key);
	        break;
		case 2:
			
			asprintf(&cOutfile,"%s.dec", cInfile);
			result = decrypt_file(cOutfile, cInfile, key);
			break;
	}
#else

	
	switch(Op)
	{
		case 0:
			printf("op error !!!\n");
			result = -1;
			break;
		case 1:
			cInfile_wo_ext = rand_filename(8);
			asprintf(&cOutfile,"%s.png", cInfile_wo_ext);

			printf("random filename : %s\n", cOutfile);

		    result = encrypt_file_pad(cOutfile, cInfile);
		
			printf("out file name : \"%s\" \n", cOutfile);
			free(cOutfile);
			free(cInfile_wo_ext);
	        break;
		case 2:
			cInfile_wo_ext = get_filename_without_extension(cInfile, NULL); 
			printf("input file without ext : %s\n", cInfile_wo_ext);
		
			asprintf(&cOutfile,"%s.dec", cInfile_wo_ext);
			result = decrypt_file_pad(cOutfile, cInfile);

			free(cOutfile);
			free(cInfile_wo_ext);
			break;
	}
#endif

	

	printf("end\n");

	return result;
}

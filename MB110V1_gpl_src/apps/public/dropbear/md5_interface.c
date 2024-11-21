/***************************************************************
 *
 * Copyright(c) 2005-2007 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	md5_interface.c
 * Version		:	1.0
 * Abstract		:	md5 make and verify response interface
 * Author		:	LI SHAOZHANG (lishaozhang@tp-link.net)
 * Created Date	:	07/11/2007
 *
 * Modified History:
 * 04Feb09, lsz add functions: md5_des and file_md5_des
 ***************************************************************/
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include "md5.h"
#include "md5_interface.h"

/*
 * md5_make_digest:make md5 digest for 'input' and save in 'digest'
 */
void md5_make_digest(unsigned char* digest, unsigned char* input, int len)
{
	MD5_CTX ctx;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, input, len);
	MD5_Final(digest, &ctx);
}

/* verify the 'digest' for 'input'*/
int md5_verify_digest(unsigned char* digest, unsigned char* input, int len)
{
	unsigned char digst[MD5_DIGEST_LEN];
	
	md5_make_digest(digst, input, len);

	if (memcmp(digst, digest, MD5_DIGEST_LEN) == 0)
		return 1;
	
	return 0;
}

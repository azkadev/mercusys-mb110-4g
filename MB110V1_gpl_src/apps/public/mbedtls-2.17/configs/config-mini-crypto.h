
#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME


#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_PKCS1_V21

#define MBEDTLS_PKCS1_V15


#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_AES_C


#define MBEDTLS_CTR_DRBG_C

#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C

#define MBEDTLS_OID_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_GENPRIME //rsa_gen_key

#define MBEDTLS_RSA_C
#define MBEDTLS_RSA_NO_CRT

#define MBEDTLS_ENTROPY_C
#define MBEDTLS_SHA256_C

#define MBEDTLS_FS_IO

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */

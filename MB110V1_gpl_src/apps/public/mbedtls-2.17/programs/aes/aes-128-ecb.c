
#define _POSIX_C_SOURCE 1

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls/platform_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#if !defined(_WIN32_WCE)
#include <io.h>
#endif
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#define MODE_ENCRYPT    0
#define MODE_DECRYPT    1

#define USAGE   \
    "\n  aes-128-ecb-encrypt <input filename> <output filename> <key>\n" \
    "\n  example: aes-128-ecb-encrypt file file.aes hex:E76B2413958B00E193\n" \
    "\n  example: aes-128-ecb-encrypt file file.aes 12345670\n" \
    "\n"

#if !defined(MBEDTLS_AES_C) || !defined(MBEDTLS_SHA256_C) || \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_MD_C) || !defined(MBEDTLS_MD5_C)
int main( void )
{
    mbedtls_printf("MBEDTLS_AES_C and/or MBEDTLS_SHA256_C "
                    "and/or MBEDTLS_FS_IO and/or MBEDTLS_MD_C and/or MBEDTLS_MD5_C "
                    "not defined.\n");
    return( 0 );
}
#else

#if defined(MBEDTLS_CHECK_PARAMS)
#include "mbedtls/platform_util.h"
void mbedtls_param_failed( const char *failure_condition,
                           const char *file,
                           int line )
{
    mbedtls_printf( "%s:%i: Input param failed - %s\n",
                    file, line, failure_condition );
    mbedtls_exit( MBEDTLS_EXIT_FAILURE );
}
#endif


#define EVP_MAX_KEY_LENGTH 64
#define EVP_MAX_MD_SIZE 64
#define PKCS5_SALT_LEN 8

int EVP_BytesToKey(const unsigned char *salt, const unsigned char *data, unsigned char *key)
{
    mbedtls_md_context_t sha_ctx;
    unsigned char md_buf[EVP_MAX_MD_SIZE]={0};
    int nkey, addmd = 0;
    unsigned int mds = 16, i, count;
    int rv = 16;
    nkey = 16;
    int datal;

    if (data == NULL)
        return (nkey);

    datal = strlen(data);
    mbedtls_md_init( &sha_ctx );
    mbedtls_md_setup( &sha_ctx, mbedtls_md_info_from_type( MBEDTLS_MD_MD5 ), 1 );
    count=0;
    for (;;) {
        count++;
        //mbedtls_printf( "count=%d\n", count);
        //mbedtls_printf( "data=%s, keylen=%d\n", data, datal);

        mbedtls_md_starts( &sha_ctx );
        if (addmd++)
            mbedtls_md_update( &sha_ctx, &(md_buf[0]), mds );
        mbedtls_md_update( &sha_ctx, data, datal );
        if (salt != NULL)
            mbedtls_md_update( &sha_ctx, salt, PKCS5_SALT_LEN );

        mbedtls_md_finish( &sha_ctx, &(md_buf[0]));
/*
        mbedtls_printf("out=");
        for (i = 0; i < 16; i++)
            mbedtls_printf("%02X", md_buf[i]);
        mbedtls_printf("\n");

        mbedtls_printf( "1 len=%d\n", mds);
*/

        i = 0;
        if (nkey) {
            for (;;) {
                if (nkey == 0)
                    break;
                if (i == mds)
                    break;
                if (key != NULL)
                    *(key++) = md_buf[i];
                nkey--;
                i++;
            }
        }

        if (nkey == 0)
            break;
    }

 err:
    mbedtls_md_free( &sha_ctx );

    return rv;
}

const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int base64_decode( const char * base64, unsigned char * bindata )
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+3] )
                temp[3]= k;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}

static char *buffpool = NULL;
static char *poolTail = NULL;
static int buffLen = 0;
int readFromBuff( char * buff,int type, int len, FILE *fin)
{
    char tmp[128];
    int base64Len;
    int readLen = 0;

    if (len > 64)
        return -1;

    if (buffpool==NULL)
    {
        buffpool = (char*)malloc(128*sizeof(char));
        memset(buffpool, 0, 128*sizeof(char));
        buffLen = 0;
        poolTail = buffpool;
    }

    if (buffLen < len)
    {
        memset(tmp, 0, 128);
        if ((readLen=fread(tmp, type, 65, fin)) != 65)
        {
            //mbedtls_printf( "read buff from file failed, len=%d\n",readLen );
            if (readLen == 0)
            {
                return 0;
            }
        }
        tmp[64] = '\0';

        base64Len = base64_decode(tmp,poolTail);
        if (base64Len < 0)
        {
            mbedtls_printf( "base64 error\n" );
            return -1;
        }
        poolTail += base64Len;
        buffLen += base64Len;
    }

    memcpy(buff, buffpool, len);
    buffLen -= len;

    memcpy(buffpool, buffpool+len, buffLen);
    poolTail -= len;

    return len;
}



int main( int argc, char *argv[] )
{
    int ret = 0;
    int exit_code = MBEDTLS_EXIT_FAILURE;

    unsigned int i, n;
    int mode, lastn;
    size_t keylen;
    FILE *fkey, *fin = NULL, *fout = NULL;

    char *p;

    unsigned char tmp[16];
    unsigned char key[512];
    unsigned char digest[16];
    unsigned char buffer[1024];

    static const char magic[] = "Salted__";
    char mbuf[sizeof(magic) - 1];
    unsigned char salt[PKCS5_SALT_LEN];

    mbedtls_aes_context aes_ctx;

#if defined(_WIN32_WCE)
    long filesize, offset;
#elif defined(_WIN32)
       LARGE_INTEGER li_size;
    __int64 filesize, offset;
#else
      off_t filesize, offset;
#endif

    mbedtls_aes_init( &aes_ctx );

    if( ret != 0 )
    {
        mbedtls_printf( "  ! mbedtls_md_setup() returned -0x%04x\n", -ret );
        goto exit;
    }

    /*
     * Parse the command-line arguments.
     */
    if( argc != 4 )
    {
        mbedtls_printf( USAGE );

#if defined(_WIN32)
        mbedtls_printf( "\n  Press Enter to exit this program.\n" );
        fflush( stdout ); getchar();
#endif

        goto exit;
    }

    memset( key,    0, sizeof( key ) );
    memset( digest, 0, sizeof( digest ) );
    memset( buffer, 0, sizeof( buffer ) );

    if( strcmp( argv[1], argv[2] ) == 0 )
    {
        mbedtls_fprintf( stderr, "input and output filenames must differ\n" );
        goto exit;
    }

    if( ( fin = fopen( argv[1], "rb" ) ) == NULL )
    {
        mbedtls_fprintf( stderr, "fopen(%s,rb) failed\n", argv[1] );
        goto exit;
    }

    if( ( fout = fopen( argv[2], "wb+" ) ) == NULL )
    {
        mbedtls_fprintf( stderr, "fopen(%s,wb+) failed\n", argv[2] );
        goto exit;
    }

    /*
     * Read the secret key from file or command line
     */
    if( ( fkey = fopen( argv[3], "rb" ) ) != NULL )
    {
        keylen = fread( key, 1, sizeof( key ), fkey );
        fclose( fkey );
    }
    else
    {
        if( memcmp( argv[3], "hex:", 4 ) == 0 )
        {
            p = &argv[3][4];
            keylen = 0;

            while( sscanf( p, "%02X", &n ) > 0 &&
                   keylen < (int) sizeof( key ) )
            {
                key[keylen++] = (unsigned char) n;
                p += 2;
            }
        }
        else
        {
            keylen = strlen( argv[3] );

            if( keylen > (int) sizeof( key ) )
                keylen = (int) sizeof( key );

            memcpy( key, argv[3], keylen );
        }
    }

#if defined(_WIN32_WCE)
    filesize = fseek( fin, 0L, SEEK_END );
#else
#if defined(_WIN32)
    /*
     * Support large files (> 2Gb) on Win32
     */
    li_size.QuadPart = 0;
    li_size.LowPart  =
        SetFilePointer( (HANDLE) _get_osfhandle( _fileno( fin ) ),
                        li_size.LowPart, &li_size.HighPart, FILE_END );

    if( li_size.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR )
    {
        mbedtls_fprintf( stderr, "SetFilePointer(0,FILE_END) failed\n" );
        goto exit;
    }

    filesize = li_size.QuadPart;
#else
    if( ( filesize = lseek( fileno( fin ), 0, SEEK_END ) ) < 0 )
    {
        perror( "lseek" );
        goto exit;
    }
#endif
#endif

    if( fseek( fin, 0, SEEK_SET ) < 0 )
    {
        mbedtls_fprintf( stderr, "fseek(0,SEEK_SET) failed\n" );
        goto exit;
    }


    //解密
    int lastPart = 0;
    /*
     *  The encrypted file must be structured as follows:
     *
     *        00 .. 15              Initialization Vector
     *        16 .. 31              AES Encrypted Block #1
     *           ..
     *      N*16 .. (N+1)*16 - 1    AES Encrypted Block #N
     *  (N+1)*16 .. (N+1)*16 + 32   HMAC-SHA-256(ciphertext)
     */
    if( filesize < 48 )
    {
        mbedtls_fprintf( stderr, "File too short to be encrypted.\n" );
        goto exit;
    }

    //获取magic字符串
    if (readFromBuff(mbuf, 1, sizeof(mbuf), fin) != sizeof(mbuf)
        || readFromBuff(salt, 1, sizeof(salt), fin) != sizeof(salt))
    {
        mbedtls_fprintf( stderr, "error reading input file\n" );
        goto exit;
    }
    else if (memcmp(mbuf, magic, sizeof(magic) - 1))
    {
        mbedtls_fprintf(stderr, "bad magic number\n");
        goto exit;
    }

    memset( digest, 0,  16 );
    EVP_BytesToKey(salt, key, digest);

    //char testTmp[16]={0xB4,0xFA,0x5A,0xE0,0x3C,0x00,0xE8,0x86,0x6C,0x01,0xD8,0xB2,0x44,0x02,0x17,0x42};
    //memcpy( digest, testTmp, 16 );

    mbedtls_aes_setkey_dec( &aes_ctx, digest, 128 );

    /*
     * Decrypt and write the plaintext.
     */
    for( offset = 0; offset < filesize; offset += 16 )
    {
        if( (n = readFromBuff( buffer, 1, 16, fin )) != 16 )
        {
            if(n == 0)
            {
                goto exit;
            }
            mbedtls_fprintf( stderr, "fread last part (%d bytes)\n", n );
            //goto exit;
        }

         if (n > 0)
        {
            memcpy( tmp, buffer, 16 );

            mbedtls_aes_crypt_ecb( &aes_ctx, MBEDTLS_AES_DECRYPT, buffer, buffer );

            if( fwrite( buffer, 1, n, fout ) != (size_t) n )
            {
                mbedtls_fprintf( stderr, "fwrite(%d bytes) failed\n", n );
                goto exit;
            }
        }
    }

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
    if( fin )
        fclose( fin );
    if( fout )
        fclose( fout );

    /* Zeroize all command line arguments to also cover
       the case when the user has missed or reordered some,
       in which case the key might not be in argv[3]. */
    for( i = 0; i < (unsigned int) argc; i++ )
        mbedtls_platform_zeroize( argv[i], strlen( argv[i] ) );

    mbedtls_platform_zeroize( key,    sizeof( key ) );
    mbedtls_platform_zeroize( tmp,    sizeof( tmp ) );
    mbedtls_platform_zeroize( buffer, sizeof( buffer ) );
    mbedtls_platform_zeroize( digest, sizeof( digest ) );

    mbedtls_aes_free( &aes_ctx );

    return( exit_code );
}
#endif /* MBEDTLS_AES_C && MBEDTLS_SHA256_C && MBEDTLS_FS_IO */

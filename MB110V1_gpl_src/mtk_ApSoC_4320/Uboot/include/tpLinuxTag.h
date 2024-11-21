
/**************************************************************************************
* File Name  : tpLinuxTag.h
*
* Description: add tag with validation system to the firmware image file to be uploaded
*              via http
*
* Created    : 16Sep07,	Liang Qiming
**************************************************************************************/

#ifndef _TP_LINUX_TAG_H_
#define _TP_LINUX_TAG_H_


#define TAG_LEN         512
#define MAGIC_LEN		24
#define CHIP_ID_LEN		8
#define BOARD_ID_LEN    16
#define TOKEN_LEN       20
#define SIG_LEN		    128

#define CLOUD_ID_BYTE_LEN	16
#define MAGIC_NUM_LEN	20

#if 0
typedef struct
{	
	unsigned long tagVersion;			/* tag version number */   
	char 		  signiture[MAGIC_LEN];	/* magic number */
	char 		  chipId[CHIP_ID_LEN];		/* chip id */  
	char 		  boardId[BOARD_ID_LEN];	/* board id */  
	unsigned long productId;	/* product id */  
	unsigned long productVer;	/* product version */
	unsigned long addHver;		/* Addtional hardware version */
	
	unsigned char imageValidToken[TOKEN_LEN];	/* image validation token - md5 checksum */
	unsigned char rcSingature[TOKEN_LEN];	 	/* RC singature(only for vxWorks) - RSA */
	
	unsigned long kernelTextAddr; 	/* text section address of kernel */
	unsigned long kernelEntryPoint; /* entry point address of kernel */
	
	unsigned long totalImageLen;	/* the sum of kernelLen+rootfsLen+tagLen */
	
	unsigned long kernelAddress;	/* starting address (offset from the beginning of FILE_TAG) 
									 * of kernel image 
									 */
	unsigned long kernelLen;		/* length of kernel image */
	
	unsigned long rootfsAddress;	/* starting address (offset) of filesystem image */
	unsigned long rootfsLen;		/* length of filesystem image */

	unsigned long bootAddress;		/* starting address (offset) of bootloader image */
	unsigned long bootLen;			/* length of bootloader image */

	/*dbs: added */
	unsigned long swRevision;		/* software revision */
	unsigned long platformVer;		/* platform version */
	/*dbs: add end */

	unsigned long binCrc32;			/* CRC32 for bin(kernel+rootfs) */

	unsigned long reserved[14];		/* reserved for future */

	unsigned char sig[SIG_LEN];		/* signature for update */
	unsigned char resSig[SIG_LEN];	/* reserved for signature */
}LINUX_FILE_TAG;
#endif
typedef struct
{
	unsigned long tagVersion;			/* tag version number */

	unsigned char hardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char firmwareId[CLOUD_ID_BYTE_LEN];		/* FWID for cloud */
	unsigned char oemId[CLOUD_ID_BYTE_LEN]; 		/* OEMID for cloud */

	unsigned long productId;	/* product id */
	unsigned long productVer;	/* product version */
	unsigned long addHver;		/* Addtional hardware version */

	unsigned char imageValidToken[TOKEN_LEN];	/* image validation token - md5 checksum */
	unsigned char magicNum[MAGIC_NUM_LEN];		/* magic number */

	unsigned long kernelTextAddr;	/* text section address of kernel */
	unsigned long kernelEntryPoint; /* entry point address of kernel */

	unsigned long totalImageLen;	/* the sum of kernelLen+rootfsLen+tagLen */

	unsigned long kernelAddress;	/* starting address (offset from the beginning of FILE_TAG)
									 * of kernel image
									 */
	unsigned long kernelLen;		/* length of kernel image */

	unsigned long rootfsAddress;	/* starting address (offset) of filesystem image */
	unsigned long rootfsLen;		/* length of filesystem image */

	unsigned long bootAddress;		/* starting address (offset) of bootloader image */
	unsigned long bootLen;			/* length of bootloader image */

	unsigned long swRevision;		/* software revision */
	unsigned long platformVer;		/* platform version */
	unsigned long specialVer;

	unsigned long binCrc32; 		/* CRC32 for bin(kernel+rootfs) */
	
#ifdef INCLUDE_COMBINED_IMAGE
	unsigned long extraproductId;	/*extra product id */
	unsigned long extrasecondproductId; /*extra second product id */
	unsigned char extrahardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char extrasecondhardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned long extra4thproductId;
	unsigned long extra5thproductId;
	unsigned long reserved1;
#else
	unsigned long reserved1[13];	/* reserved for future */
#endif

	unsigned char sig[SIG_LEN];		/* signature for update */

#ifdef INCLUDE_COMBINED_IMAGE
	unsigned char resSig[SIG_LEN - 20 - 64];	/* reserved for signature */
	unsigned char extra4thhardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char extra5thhardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char extra4thoemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */
	unsigned char extra5thoemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */
	unsigned long extrathirdproductId; /*extra third product id */
	unsigned char extrathirdhardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
#else
	unsigned char resSig[SIG_LEN];	/* reserved for signature */
#endif

#ifdef INCLUDE_COMBINED_IMAGE
	unsigned char extraoemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */
	unsigned char extrasecondoemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */
	unsigned char extrathirdoemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */
#else
	unsigned long reserved2[12];	/* reserved for future */
#endif
}LINUX_FILE_TAG;


#define BOOTLOADER_SIZE         0x20000
#define FILE_TAG_SIZE			512

#define LINUX_KERNEL_TAG_OFFSET		(BOOTLOADER_SIZE)



#endif // _TP_LINUX_TAG_H_


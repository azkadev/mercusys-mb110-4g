#define MAX_TEST_VECTOR	10
#define MAX_TEST_SEQ	20
#define MAX_SG_BUF		8

extern unsigned char padlock_aes_c[];
extern unsigned char padlock_aes_c_aes[];

typedef struct test_pattern_s
{
	uint32_t	src_multipage;
	uint32_t	dst_multipage;
	uint32_t	src_offset[MAX_SG_BUF];
	uint32_t	dst_offset[MAX_SG_BUF];
	uint32_t	src_length[MAX_SG_BUF];
	uint32_t	dst_length[MAX_SG_BUF];
	uint32_t	totallen;
	uint32_t	plainlen;
	uint32_t	cipherlen;
	uint32_t	mode;
	const uint8_t*	key;
	const uint8_t*	iv;
	const uint8_t*	plaintext;
	const uint8_t*	ciphertext;
		
}test_pattern_t;

test_pattern_t pattern[MAX_TEST_VECTOR] = {
	/* single TX + single RX + 4 byte align */
	{
		.src_multipage = 1,
		.dst_multipage = 1,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.dst_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_128,
		.key = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
		.iv = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
		.plaintext = "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff",
		.ciphertext = "\x69\xc4\xe0\xd8\x6a\x7b\x04\x30\xd8\xcd\xb7\x80\x70\xb4\xc5\x5a"
	},
	{
		.src_multipage = 1,
		.dst_multipage = 1,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.dst_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_128|CBC_MODE,
		.key = "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c",
		.iv = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
		.plaintext = "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
		.ciphertext = "\x76\x49\xab\xac\x81\x19\xb2\x46\xce\xe9\x8e\x9b\x12\xe9\x19\x7d",
	},
		{
		.src_multipage = 1,
		.dst_multipage = 1,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {1, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {16, 10, 0, 0, 0, 0, 0, 0},
		.dst_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_192|CBC_MODE,
		.key = "\x8e\x73\xb0\xf7\xda\x0e\x64\x52\xc8\x10\xf3\x2b\x80\x90\x79\xe5\x62\xf8\xea\xd2\x52\x2c\x6b\x7b",
		.iv = "\x4F\x02\x1D\xB2\x43\xBC\x63\x3D\x71\x78\x18\x3A\x9F\xA0\x71\xE8",
		.plaintext = "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
		.ciphertext = "\xb4\xd9\xad\xa9\xad\x7d\xed\xf4\xe5\xe7\x38\x76\x3f\x69\x14\x5a",
	},
	{
		.src_multipage = 1,
		.dst_multipage = 1,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.dst_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_256|CBC_MODE,
		.key = "\x60\x3d\xeb\x10\x15\xca\x71\xbe\x2b\x73\xae\xf0\x85\x7d\x77\x81\x1f\x35\x2c\x07\x3b\x61\x08\xd7\x2d\x98\x10\xa3\x09\x14\xdf\xf4",
		.iv = "\x39\xF2\x33\x69\xA9\xD9\xBA\xCF\xA5\x30\xE2\x63\x04\x23\x14\x61",
		.plaintext = "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
		.ciphertext = "\xb2\xeb\x05\xe2\xc3\x9b\xe9\xfc\xda\x6c\x19\x07\x8c\x6a\x9d\x1b",
	},
	/* Multi TX + Multi RX + 4 byte align */
	{
		.src_multipage = 2,
		.dst_multipage = 2,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {4, 12, 0, 0, 0, 0, 0, 0},
		.dst_length = {8, 8, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_128,
		.key = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
		.iv = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
		.plaintext = "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff",
		.ciphertext = "\x69\xc4\xe0\xd8\x6a\x7b\x04\x30\xd8\xcd\xb7\x80\x70\xb4\xc5\x5a"
	},
	{
		.src_multipage = 2,
		.dst_multipage = 4,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {6, 10, 0, 0, 0, 0, 0, 0},
		.dst_length = {4, 4, 4, 4, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_128|CBC_MODE,
		.key = "\x2b\x7e\x15\x16\x28\xae\xd2\xa6\xab\xf7\x15\x88\x09\xcf\x4f\x3c",
		.iv = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F",
		.plaintext = "\x6b\xc1\xbe\xe2\x2e\x40\x9f\x96\xe9\x3d\x7e\x11\x73\x93\x17\x2a",
		.ciphertext = "\x76\x49\xab\xac\x81\x19\xb2\x46\xce\xe9\x8e\x9b\x12\xe9\x19\x7d",
	},
	/* Multi TX any byte align + single RX any byte align */
	{
		.src_multipage = 2,
		.dst_multipage = 1,
		.src_offset = {1, 3, 0, 0, 0, 0, 0, 0},
		.dst_offset = {1, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {6, 10, 0, 0, 0, 0, 0, 0},
		.dst_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_192|CBC_MODE,
		.key = "\x8e\x73\xb0\xf7\xda\x0e\x64\x52\xc8\x10\xf3\x2b\x80\x90\x79\xe5\x62\xf8\xea\xd2\x52\x2c\x6b\x7b",
		.iv = "\x4F\x02\x1D\xB2\x43\xBC\x63\x3D\x71\x78\x18\x3A\x9F\xA0\x71\xE8",
		.plaintext = "\xae\x2d\x8a\x57\x1e\x03\xac\x9c\x9e\xb7\x6f\xac\x45\xaf\x8e\x51",
		.ciphertext = "\xb4\xd9\xad\xa9\xad\x7d\xed\xf4\xe5\xe7\x38\x76\x3f\x69\x14\x5a",
	},
	{
		.src_multipage = 1,
		.dst_multipage = 1,
		.src_offset = {2, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {3, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.dst_length = {16, 0, 0, 0, 0, 0, 0, 0},
		.totallen = 16,
		.plainlen = 16,
		.cipherlen = 16,
		.mode = AES_256|CBC_MODE,
		.key = "\x60\x3d\xeb\x10\x15\xca\x71\xbe\x2b\x73\xae\xf0\x85\x7d\x77\x81\x1f\x35\x2c\x07\x3b\x61\x08\xd7\x2d\x98\x10\xa3\x09\x14\xdf\xf4",
		.iv = "\x39\xF2\x33\x69\xA9\xD9\xBA\xCF\xA5\x30\xE2\x63\x04\x23\x14\x61",
		.plaintext = "\xf6\x9f\x24\x45\xdf\x4f\x9b\x17\xad\x2b\x41\x7b\xe6\x6c\x37\x10",
		.ciphertext = "\xb2\xeb\x05\xe2\xc3\x9b\xe9\xfc\xda\x6c\x19\x07\x8c\x6a\x9d\x1b",
	},
/*	
	{
		.src_multipage = 6,
		.dst_multipage = 6,
		.src_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {2592, 2592, 2592, 2592, 2592, 2448, 0, 0},
		.dst_length = {2592, 2592, 2592, 2592, 2592, 2448, 0, 0},
		.totallen = 15397,
		.plainlen = 15397,
		.cipherlen = 15408,
		.mode = AES_128|CBC_MODE,
		//.mode = AES_256|CBC_MODE,
		.key = "\x25\xF9\xE7\x94\x32\x3B\x45\x38\x85\xF5\x18\x1F\x1B\x62\x4D\x0B",
		.iv =  "\x5A\x5A\xA5\xA5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		.plaintext = padlock_aes_c,
		.ciphertext = padlock_aes_c_aes
	},
	{
		.src_multipage = 6,
		.dst_multipage = 6,
		.src_offset = {2, 3, 4, 5, 6, 7, 0, 0},
		.dst_offset = {0, 0, 0, 0, 0, 0, 0, 0},
		.src_length = {2591, 2593, 2593, 2591, 2593, 2447, 0, 0},
		.dst_length = {2591, 2593, 2592, 2592, 2592, 2448, 0, 0},
		.totallen = 15397,
		.plainlen = 15397,
		.cipherlen = 15408,
		.mode = AES_128|CBC_MODE,
		//.mode = AES_256|CBC_MODE,
		.key = "\x25\xF9\xE7\x94\x32\x3B\x45\x38\x85\xF5\x18\x1F\x1B\x62\x4D\x0B",
		.iv =  "\x5A\x5A\xA5\xA5\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		.plaintext = padlock_aes_c,
		.ciphertext = padlock_aes_c_aes
	}
*/	
};	



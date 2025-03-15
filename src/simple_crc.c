/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>



static uint32_t g_crc_slicing_initialed = 0;
static uint32_t g_crc_slicing[8][256];

#define CRCPOLY 0xEDB88320 // reversed 0x1EDC6F41

static void Simple_CRC_Slicing_Init(void) 
{
	if(g_crc_slicing_initialed == 0)
	{
		uint32_t i, j;
		
		for (i = 0; i <= 0xFF; i++) 
		{
			uint32_t x = i;
			for (j = 0; j < 8; j++)
				x = (x>>1) ^ (CRCPOLY & (-(int)(x & 1)));
			g_crc_slicing[0][i] = x;
		}

		for (i = 0; i <= 0xFF; i++) 
		{
			uint32_t c = g_crc_slicing[0][i];
			for (j = 1; j < 8; j++) {
				c = g_crc_slicing[0][c & 0xFF] ^ (c >> 8);
				g_crc_slicing[j][i] = c;
			}
		}
	
		g_crc_slicing_initialed = 1;

/*
		do
		{
		uint32_t k;
		for(k=0;k<8;k++)  // slice table
		{
			printf("{   \n");
			for(j=0;j<32;j++)  // table lines
			{
				for(i=0;i<8;i++)
				{
					printf("0x%08X, ", g_crc_slicing[k][(j*8) + i]);
				}
				printf("\n");	
			}

			printf("},   \n");
		}
		}while(0);
*/
		
	}
}


uint32_t Simple_CRC_SlicingBy8(uint32_t StartCRC, const uint8_t* buf, unsigned long len) 
{
	uint32_t crc = StartCRC^0xFFFFFFFF;

	// Align to DWORD boundary
	unsigned long align = (sizeof(uint32_t) - (intptr_t)buf) & (sizeof(uint32_t) - 1);
	align = align < len ? align: len ;  //Min(align, len);
	len -= align;

	Simple_CRC_Slicing_Init();
	
	for (; align; align--)
		crc = g_crc_slicing[0][(crc ^ *buf++) & 0xFF] ^ (crc >> 8);

	unsigned long nqwords = len / (sizeof(uint32_t) + sizeof(uint32_t));
	for (; nqwords; nqwords--) {
		crc ^= *(uint32_t*)buf;
		buf += sizeof(uint32_t);
		uint32_t next = *(uint32_t*)buf;
		buf += sizeof(uint32_t);
		crc =
			g_crc_slicing[7][(crc      ) & 0xFF] ^
			g_crc_slicing[6][(crc >>  8) & 0xFF] ^
			g_crc_slicing[5][(crc >> 16) & 0xFF] ^
			g_crc_slicing[4][(crc >> 24)] ^
			g_crc_slicing[3][(next      ) & 0xFF] ^
			g_crc_slicing[2][(next >>  8) & 0xFF] ^
			g_crc_slicing[1][(next >> 16) & 0xFF] ^
			g_crc_slicing[0][(next >> 24)];
	}

	len &= sizeof(uint32_t) * 2 - 1;
	for (; len; len--)
		crc = g_crc_slicing[0][(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
		
	return ~crc;
}


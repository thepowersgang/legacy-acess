/*
AcessOS 1.0
DMA Driver
*/
#include <acess.h>

#define DMA_SIZE	(0x2400)
#define DMA_ADDRESS(c)	((c)*DMA_SIZE+0x500)	//Save Space for IDT and BDA

typedef struct {
	int 	mode;
	char	*address;
} t_dmaChannel;


const Uint8 cMASKPORT [8] = { 0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4 };
const Uint8 cMODEPORT [8] = { 0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6 };
const Uint8 cCLEARPORT[8] = { 0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8 };
const Uint8 cPAGEPORT [8] = { 0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A };
const Uint8 cADDRPORT [8] = { 0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC };
const Uint8 cCOUNTPORT[8] = { 0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE };

char	*dma_addresses[8];
t_dmaChannel	dma_channels[8];

/*
Initialise DMA channels
*/
void dma_install()
{
	int i;
	for(i=8;i--;)
	{
		outportb( cMASKPORT[i], 0x04 | (i & 0x3) ); // mask channel
		outportb( cCLEARPORT[i], 0x00 );
		outportb( cMODEPORT[i], 0x48 | (i & 0x3) );	//Read Flag
		outportb( 0xd8, 0xff);	//Reset Flip-Flop
		outportb( cADDRPORT[i], LOWB(DMA_ADDRESS(i)) );	// send address
		outportb( cADDRPORT[i], HIB(DMA_ADDRESS(i)) );	// send address
		outportb( 0xd8, 0xff);	//Reset Flip-Flop
		outportb( cCOUNTPORT[i], LOWB(DMA_SIZE) );      // send size
		outportb( cCOUNTPORT[i], HIB(DMA_SIZE) );       // send size
		outportb( cPAGEPORT[i], LOWB(HIW(DMA_ADDRESS(i))) );	// send page
		outportb( cMASKPORT[i], i & 0x3 );              // unmask channel
		
		dma_channels[i].mode = 0;
		dma_addresses[i] = (char*)DMA_ADDRESS(i);
		dma_addresses[i] += 0xC0000000;
	}
}

/*
void dma_setChannel(int channel, int length, int read)
- Set DMA Channel Length and RW
*/
void dma_setChannel(int channel, int length, int read)
{
	channel &= 7;
	read = read && 1;
	if(length > DMA_SIZE)	length = DMA_SIZE;
	length --;	//Adjust for DMA
	//__asm__ __volatile__ ("cli");
	outportb( cMASKPORT[channel], 0x04 | (channel & 0x3) );		// mask channel
	outportb( cCLEARPORT[channel], 0x00 );
	outportb( cMODEPORT[channel], (0x44 + (!read)*4) | (channel & 0x3) );
	outportb( cADDRPORT[channel], LOWB(DMA_ADDRESS(channel)) );		// send address
	outportb( cADDRPORT[channel], HIB(DMA_ADDRESS(channel)) );		// send address
	outportb( cPAGEPORT[channel], HIW(DMA_ADDRESS(channel)) );		// send page
	outportb( cCOUNTPORT[channel], LOWB(length) );      // send size
	outportb( cCOUNTPORT[channel], HIB(length) );       // send size
	outportb( cMASKPORT[channel], channel & 0x3 );              // unmask channel
	dma_addresses[channel] = (char*)DMA_ADDRESS(channel);
	dma_addresses[channel] += 0xC0000000;
	//__asm__ __volatile__ ("sti");
}

/*
void dma_readData(int channel, int count, void *buffer)
*/
int dma_readData(int channel, int count, void *buffer)
{
	if(channel < 0 || channel > 7)
		return -1;
	if(count < 0 || count > DMA_SIZE)
		return -2;
	//LogF("memcpy(*0x%x, dma_channels[channel].address, count)\n", buffer
	memcpy(buffer, dma_addresses[channel], count);
	return 0;
}

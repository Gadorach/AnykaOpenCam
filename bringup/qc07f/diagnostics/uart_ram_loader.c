/* SPDX-License-Identifier: GPL-2.0-only
 * QC07F RAM-only stop-and-wait UART loader, called by unchanged U-Boot go.
 * REA-retained serial_device callbacks: getc80e14750, tstc80e14820.
 * Compile with fixedr9 to retain U-Boot's global-data convention. No SPI/GPIO
 * writes. Destination82408000..82808000 only. A15s keyed watchdog timeout
 * recovers a stalled transfer; stop it before returning to the held CLI.
 * First-byte wait is unarmed, so the host can inspect READY or cancel with ESC.
 * Header: QCL1 + little-endian(total length, full CRC32/BZIP2).
 * Frame: little-endian(seq, length<=96, packet CRC) + payload.
 * Duplicate last frame is acknowledged without advancing full CRC twice.
 */
#include "mmu_uart.h"
#define DEST 0x82408000U
#define LIMIT 0x00400000U
static int get(void) { return ((int(*)(void))0x80e14750U)() & 255; }
static int ready(void) { return ((int(*)(void))0x80e14820U)(); }
static void feed(void)
{
	diag_write(0x080000e4,0x55000000U|(15U*46875U));
	diag_write(0x080000e8,0xaa000003U);
}
static void stop(void)
{
	diag_write(0x080000e8,0xaa000000U);
	diag_write(0x080000e4,0x55ffffffU);
}
static u32 crc_byte(u32 crc,unsigned char ch)
{
	u32 bit;crc^=(u32)ch<<24;
	for(bit=0;bit<8;bit++)crc=(crc<<1)^((crc&0x80000000U)?0x04c11db7U:0);
	return crc;
}
static u32 receive_word(void)
{
	u32 word=0,shift;
	for(shift=0;shift<32;shift+=8)word|=(u32)get()<<shift;
	return word;
}
static void ack(const char *status,u32 seq)
{
	diag_text(status);diag_hex(seq);diag_text("\r\n");
}
__attribute__((section(".text.entry")))
int _start(int argc,char **argv)
{
	u32 total,expected,offset=0,sequence=0,full=0xffffffffU,last_crc=0,last_length=0;
	unsigned char packet[96];int first;u32 i;
	if(diag_read(0x080000e8)&1){diag_text("QCL_BUSY_WATCHDOG\r\n");return 1;}
	diag_text("\r\nQCL_READY_1 RAM_ONLY\r\n");
	while(!ready()){}
	first=get();if(first==27){diag_text("QCL_CANCEL\r\n");return 1;}
	feed();
	if(first!='Q' || get()!='C' || get()!='L' || get()!='1')
		diag_halt("QCL_BAD_HEADER");
	total=receive_word();expected=receive_word();
	if(total<64 || total>LIMIT)diag_halt("QCL_BAD_LENGTH");
	diag_text("QCL_BEGIN ");diag_hex(total);diag_text("\r\n");
	while(offset<total){
		u32 seq=receive_word(),len=receive_word(),want=receive_word(),crc=0xffffffffU;
		if(!len || len>96)diag_halt("QCL_BAD_FRAME_LENGTH");
		for(i=0;i<len;i++){packet[i]=(unsigned char)get();crc=crc_byte(crc,packet[i]);}
		feed();crc^=0xffffffffU;
		if(crc!=want){ack("QCL_NAK ",seq);continue;}
		if(sequence && seq==sequence-1 && crc==last_crc && len==last_length){ack("QCL_ACK ",seq);continue;}
		if(seq!=sequence || len>total-offset){ack("QCL_NAK ",seq);continue;}
		for(i=0;i<len;i++){
			*(volatile unsigned char *)(DEST+offset+i)=packet[i];
			full=crc_byte(full,packet[i]);
		}
		offset+=len;last_crc=crc;last_length=len;sequence++;ack("QCL_ACK ",seq);
	}
	full^=0xffffffffU;
	diag_text(full==expected?"QCL_DONE ":"QCL_BAD_CRC ");
	diag_hex(total);diag_text(" ");diag_hex(full);diag_text("\r\n");
	if(full!=expected)diag_halt("QCL_TRANSFER_REJECTED");
	/* Keep binary input away from the CLI until the host commits. A duplicate
	 * last frame can be retried safely even if its ACK was lost.
	 */
	for(;;){
		u32 token=receive_word(),len,want,crc=0xffffffffU;
		if(token==0x444e4551U)break; /* QEND */
		len=receive_word();want=receive_word();
		if(!len || len>96)diag_halt("QCL_BAD_FINISH_LENGTH");
		for(i=0;i<len;i++){packet[i]=(unsigned char)get();crc=crc_byte(crc,packet[i]);}
		feed();crc^=0xffffffffU;
		if(token==sequence-1 && len==last_length && want==crc && crc==last_crc){
			ack("QCL_ACK ",token);diag_text("QCL_DONE ");diag_hex(total);
			diag_text(" ");diag_hex(full);diag_text("\r\n");
		}else ack("QCL_NAK ",token);
	}
	stop();diag_text("QCL_COMMIT_OK\r\n");
	return full==expected?0:1;
}

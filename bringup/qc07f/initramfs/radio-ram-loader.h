/* SPDX-License-Identifier: GPL-2.0-only
 * New bounded loader, REA-correlated retail bus/bootdl semantics. Only exact
 * chip8400 payload and radio RAM20000000..20024e10. No main CPU MMIO/SPI,
 * OTA, OTP/eFuse or bootloader register-write opcode. User-command only.
 */
#include "radio-payload.h"
static void radio_word(unsigned char *p,u32 v)
{p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
static long radio_bulk(long fd,u32 endpoint,void *data,u32 bytes,u32 timeout)
{
	struct usb_bulk_wire transfer={endpoint,bytes,timeout,data};
	return sc(54,fd,0xc0105502,(long)&transfer,0,0);
}
static long radio_reset_toggles(long fd)
{
	u32 endpoint=1;long result=sc(54,fd,0x80045515,(long)&endpoint,0,0);
	if(result<0)return result;
	endpoint=0x81;return sc(54,fd,0x80045515,(long)&endpoint,0,0);
}
static long radio_boot_response(long fd,u32 cookie,u32 command,u32 *value)
{
	unsigned char response[512];long n;u32 attempt;
	for(attempt=0;attempt<4;attempt++){
		n=radio_bulk(fd,0x81,response,sizeof(response),500);if(n<0)return n;
		out("RADIO BOOT RX cmd0x");hex(command);out(" cookie0x");hex(cookie);out(" len0x");hex(n);out(" header ");
		{u32 i;for(i=0;i<(u32)n && i<16;i++){char pair[3];pair[0]="0123456789abcdef"[response[i]>>4];pair[1]="0123456789abcdef"[response[i]&15];pair[2]=0;out(pair);}out("\n");}
		if(n<16 || response[0]!=0x1a || response[1]!=0x2b || response[6]!=(cookie&255) || response[7]!=(cookie>>8))continue;
		/* Raw data ACK uses retained cookie/status; stock send_fw does not
		 * require a particular command byte, unlike command acknowledgments.
		 */
		if((command!=0xffffffff && response[8]!=command) || response[15]!=0xfd)return -5;
		if(value)*value=(u32)response[11]|((u32)response[12]<<8)|((u32)response[13]<<16)|((u32)response[14]<<24);
		return response[9];
	}
	return -110;
}
static int radio_native_transport;
static long radio_boot_command(long fd,u32 command,u32 cookie,u32 address,u32 size)
{
	unsigned char packet[20]={0x2b,0x1a,7,0,20,0,0,0,0,12,2,0,0,0,0,0,0,0,0,0xfd};
	u32 i;long result;
	if(!radio_native_transport){result=radio_reset_toggles(fd);if(result<0)return result;}
	packet[6]=cookie;packet[7]=cookie>>8;packet[8]=command;
	if(command==0){const char *key="@huge-ic";for(i=0;i<8;i++)packet[11+i]=key[i];}
	else{radio_word(packet+11,address);radio_word(packet+15,size);}
	result=radio_bulk(fd,1,packet,sizeof(packet),500);
	if(result!=20)return result<0?result:-5;
	return radio_boot_response(fd,cookie,command,0);
}
static void radio_load(int native)
{
	long fd=-1,guard=-1,result;int timeout=5,claimed=0;u32 interface=0;
	u32 bytes=(u32)(radio_payload_end-radio_payload_begin),offset=0,chunk,wire_bytes,cookie=100,i,bit,crc=0xffffffff;
	unsigned char wire[2052];const unsigned char *data;
	char ping='.',magic='V';
	radio_native_transport=native;
	if(bytes!=151056){out("Refuse radio load: payload length\n");return;}
	for(i=0;i<bytes;i++){
		crc^=radio_payload_begin[i];for(bit=0;bit<8;bit++)crc=(crc>>1)^((crc&1)?0xedb88320:0);
	}
	if(crc){out("Refuse radio load: exact raw CRC residue differs\n");return;}
	if(!sysfs_prefix_value("/sys/bus/usb/devices/1-1/idVendor","a012") ||
	   !sysfs_prefix_value("/sys/bus/usb/devices/1-1/idProduct","8400") ||
	   !sysfs_prefix_value("/sys/bus/usb/devices/1-1/bConfigurationValue","1") || !watchdog_stoppable()){
		out("Refuse radio load: owned configured device/guard not confirmed\n");return;
	}
	fd=radio_dev_open();if(fd<0){out("Radio USB node unavailable\n");return;}
	guard=sc(5,(long)"/dev/watchdog0",2,0,0,0);if(guard<0)goto cleanup;
	result=sc(54,guard,0xc0045706,(long)&timeout,0,0);if(result<0 || timeout!=5)goto cleanup;
	result=sc(54,fd,0x8004550f,(long)&interface,0,0);if(result<0)goto cleanup;claimed=1;
	result=radio_boot_command(fd,0,cookie++,0,0);
	if(result!=0){out("Refuse radio load: validated BOOT state not0, result0x");hex(result);out("\n");goto cleanup;}
	/* GET_SC is a read-only security/capability query. Do not bypass rejection. */
	result=radio_boot_command(fd,1,cookie++,0,0);
	if(result!=0){out("Refuse radio load: security query rejected0x");hex(result);out("\n");goto cleanup;}
	out("RADIO EXACT RETAIL RAM LOAD BEGIN20000000..20024e10 AESoff CRCraw0; no SPI/OTA/OTP\n");
	while(offset<bytes){
		/*2048 is the retail maximum, not a minimum. Single256-byte packets
		 * isolate the current host's multi-packet state failure; no padding,
		 * changed image, protection override or extended destination range.
		 */
		chunk=bytes-offset;if(chunk>(native?2048U:256U))chunk=native?2048U:256U;
		/* Retail USB command declares chunk+1024; actual data is exactly
		 * chunk raw firmware bytes. hgics_tx_work strips virtual type15's
		 *20-byte header before USB, and URB_ZERO_PACKET ends512 multiples.
		 */
		result=radio_boot_command(fd,2,cookie,0x20000000+offset,chunk+1024);
		if(result!=0)goto failed;
		/* Source40 confirms repeated exchanges only with both endpoint
		 * toggles synchronized. This is standard USB endpoint recovery,
		 * not a radio protection/status-register or firmware-profile change.
		 */
		if(!native){result=radio_reset_toggles(fd);if(result<0)goto failed;}
		/* Actual retained hgic_usb_tx_packet (REA0x1b3c8) adds four
		 * transport bytes to512-byte multiples. This is NOT payload padding:
		 * the exact firmware/CRC/offset remain unchanged, and later fragments
		 * overwrite those temporary transport-tail locations as retail does.
		 */
		data=radio_payload_begin+offset;wire_bytes=chunk;
		if(!(chunk%512)){
			for(i=0;i<chunk;i++)wire[i]=data[i];
			for(i=0;i<4;i++)wire[chunk+i]=0;
			data=wire;wire_bytes+=4;
		}
		out("RADIO DATA OUT BEGIN payload0x");hex(chunk);out(" wire0x");hex(wire_bytes);out("\n");
		result=radio_bulk(fd,1,(void *)data,wire_bytes,1000);
		out("RADIO DATA OUT RESULT0x");hex(result);out("\n");
		if(result!=(long)wire_bytes){if(result>=0)result=-5;goto failed;}
		if(!(wire_bytes%512)){
			out("RADIO ZLP BEGIN\n");
			result=radio_bulk(fd,1,(void *)radio_payload_begin,0,500);if(result<0)goto failed;
			out("RADIO ZLP RESULT0x");hex(result);out("\n");
		}
		result=radio_boot_response(fd,cookie,0xffffffff,0);if(result!=0)goto failed;
		offset+=chunk;cookie++;sc(4,guard,(long)&ping,1,0,0);
		if(!(cookie&7)){out("RADIO RAM BYTES ACCEPTED0x");hex(offset);out("\n");}
	}
	/* RUN only this same declared radio RAM base, exact size, CRC-check and
	 * standard SPI-config flags from the retained stock loader. No flash write.
	 */
	result=radio_boot_command(fd,4,cookie,0x20000000,bytes|0x60000000);
	if(result!=0)goto failed;
	out("RADIO RAM LOAD/CRC/RUN ACKNOWLEDGED; application/ABI/association still require verification\n");goto cleanup;
failed:
	out("RADIO RAM LOAD FAILED offset0x");hex(offset);out(" result0x");hex(result);out("; do not infer running firmware, recover by GPIO42 cold reset\n");
cleanup:
	if(claimed)sc(54,fd,0x80045510,(long)&interface,0,0);
	if(guard>=0){sc(4,guard,(long)&magic,1,0,0);sc(6,guard,0,0,0,0);}
	if(fd>=0)sc(6,fd,0,0,0,0);
}

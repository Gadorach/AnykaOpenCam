/* SPDX-License-Identifier: GPL-2.0-only
 * Freestanding ARM EABI PID1 bring-up monitor. No libc, cloud code, flash tools
 * or motor actuation. Explicit build modes add bounded peripheral/sensor tests.
 */
typedef unsigned int u32;
typedef unsigned long usize;
static long sc(long n, long a, long b, long c, long d, long e)
{
	register long r0 __asm__("r0") = a, r1 __asm__("r1") = b;
	register long r2 __asm__("r2") = c, r3 __asm__("r3") = d;
	register long r4 __asm__("r4") = e, r7 __asm__("r7") = n;
	__asm__ volatile("svc 0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r7) : "memory", "cc");
	return r0;
}
static usize length(const char *s) { usize n = 0; while (s[n]) n++; return n; }
static int equal(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int prefix(const char *s, const char *p) { while (*p) if (*s++ != *p++) return 0; return 1; }
static void out(const char *s) { sc(4, 1, (long)s, length(s), 0, 0); }
static void hex(u32 value)
{
	char text[9]; unsigned int i;
	for(i=0;i<8;i++) text[i]="0123456789abcdef"[(value>>((7-i)*4))&15];
	text[8]=0;out(text);
}
#ifdef QC07F_PERIPHERAL_QUALIFICATION
/* Exact32-bit UAPI wire layouts. GPIO inspection never requests a line,
 * changes pin direction/bias or issues value-setting ioctls.
 */
typedef unsigned long long u64;
struct gpio_chip_info_wire { char name[32], label[32]; u32 lines; };
struct gpio_attribute_wire { u32 id, padding; u64 value __attribute__((aligned(8))); };
struct gpio_line_info_wire {
	char name[32], consumer[32]; u32 offset, num_attrs;
	u64 flags __attribute__((aligned(8)));
	struct gpio_attribute_wire attrs[10]; u32 padding[4];
};
_Static_assert(sizeof(struct gpio_chip_info_wire)==68,"GPIO chip UAPI");
_Static_assert(sizeof(struct gpio_line_info_wire)==256,"GPIO line UAPI");
static void zero(void *p,usize n)
{
	volatile unsigned char *bytes=p;
	while(n--) *bytes++=0;
}
static void gpio_info(void)
{
	struct gpio_chip_info_wire chip;struct gpio_line_info_wire line;
	long fd,result;u32 i;
	fd=sc(5,(long)"/dev/gpiochip0",0,0,0,0);
	if(fd<0){out("GPIO open failed\n");return;}
	zero(&chip,sizeof(chip));result=sc(54,fd,0x8044b401,(long)&chip,0,0);
	if(result<0){out("GPIO chip info failed\n");goto close;}
	chip.name[31]=0;chip.label[31]=0;
	out("GPIO name=");out(chip.name);out(" label=");out(chip.label);
	out(" lines=0x");hex(chip.lines);out("\n");
	if(chip.lines!=79){out("Unexpected line count; inspection stopped\n");goto close;}
	for(i=0;i<chip.lines;i++){
		zero(&line,sizeof(line));line.offset=i;
		result=sc(54,fd,0xc100b405,(long)&line,0,0);
		if(result<0){out("GPIO line info failed\n");break;}
		out("GPIO offset=0x");hex(i);out(" flags=0x");
		hex((u32)(line.flags>>32));hex((u32)line.flags);out("\n");
	}
close:
	sc(6,fd,0,0,0,0);
}
static int watchdog_stoppable(void)
{
	long fd,n;char value[4];
	fd=sc(5,(long)"/sys/class/watchdog/watchdog0/nowayout",0,0,0,0);
	if(fd<0)return 0;
	n=sc(3,fd,(long)value,sizeof(value),0,0);sc(6,fd,0,0,0,0);
	return n>0 && value[0]=='0';
}
static void sleep_seconds(long seconds)
{
	long t[2]={seconds,0};sc(162,(long)t,0,0,0,0);
}
static void watchdog_test(int reset)
{
	long fd,result;int timeout=reset?1:5, actual=0;char magic='V',ping='.';
	if(!watchdog_stoppable()){out("Refuse watchdog test: nowayout not confirmed0\n");return;}
	out("Explicit watchdog register test; no GPIO or SPI writes\n");
	fd=sc(5,(long)"/dev/watchdog0",2,0,0,0);
	if(fd<0){out("Watchdog open failed\n");return;}
	result=sc(54,fd,0xc0045706,(long)&timeout,0,0);
	if(result<0 || timeout!=(reset?1:5)){
		out("Watchdog set-timeout failed\n");goto stop;
	}
	if(reset){
		out("WATCHDOG RESET ARMED: nominal1s; fd remains open, no keepalive\n");
		for(;;)sleep_seconds(1);
	}
	sleep_seconds(2);
	result=sc(4,fd,(long)&ping,1,0,0);
	if(result!=1){out("Watchdog ping failed\n");goto stop;}
	out("WATCHDOG PING ACCEPTED\n");sleep_seconds(2);
	result=sc(54,fd,0x80045707,(long)&actual,0,0);
	if(result<0 || actual!=5){out("Watchdog get-timeout failed\n");goto stop;}
	out("WATCHDOG TIMEOUT VERIFIED5\n");
stop:
	result=sc(4,fd,(long)&magic,1,0,0);
	if(result!=1)out("Watchdog magic-close write failed; reset may follow\n");
	result=sc(6,fd,0,0,0,0);
	if(result==0)out("WATCHDOG MAGIC CLOSE COMPLETED\n");
}
#endif
#ifdef QC07F_I2C_QUALIFICATION
#include "sc1346-init.h"
struct i2c_msg_wire { unsigned short addr,flags,len,padding; unsigned char *buffer; };
struct i2c_rdwr_wire { struct i2c_msg_wire *messages; u32 count; };
_Static_assert(sizeof(struct i2c_msg_wire)==12,"ARM32 I2C msg UAPI");
_Static_assert(sizeof(struct i2c_rdwr_wire)==8,"ARM32 I2C rdwr UAPI");
static long sensor_message(long fd,unsigned char *buffer,unsigned short length,unsigned short flags)
{
	struct i2c_msg_wire msg;struct i2c_rdwr_wire request;
	msg.addr=0x30;msg.flags=flags;msg.len=length;msg.padding=0;msg.buffer=buffer;
	request.messages=&msg;request.count=1;
	return sc(54,fd,0x0707,(long)&request,0,0);
}
static long sensor_read(long fd,unsigned short reg)
{
	unsigned char address[2],value=0;long result;
	address[0]=reg>>8;address[1]=reg;
	result=sensor_message(fd,address,2,0);
	if(result!=1)return result<0?result:-5;
	result=sensor_message(fd,&value,1,1);
	return result==1?value:(result<0?result:-5);
}
static long sensor_write(long fd,unsigned short reg,unsigned char value)
{
	unsigned char data[3];long result;
	data[0]=reg>>8;data[1]=reg;data[2]=value;
	result=sensor_message(fd,data,3,0);return result==1?0:(result<0?result:-5);
}
static int sensor_validate(long fd)
{
	long high=sensor_read(fd,0x3107),low=sensor_read(fd,0x3108);
	if(high<0 || low<0 || (((u32)high<<8)|(u32)low)!=0xda4d){
		out("Refuse sensor writes: silicon ID not confirmedda4d\n");return 0;
	}
	return 1;
}
static void sensor_state_fd(long fd)
{
	const unsigned short regs[]={0x0100,0x3208,0x3209,0x320a,0x320b,0x320c,0x320d,0x320e,0x320f};
	u32 i;long value;
	for(i=0;i<sizeof(regs)/sizeof(regs[0]);i++){
		value=sensor_read(fd,regs[i]);out("SENSOR REG 0x");hex(regs[i]);
		out(" VALUE 0x");hex(value);out("\n");
		if(value<0)break;
	}
}
static void sensor_control(int operation)
{
	long fd,result=0;u32 i;
	fd=sc(5,(long)"/dev/i2c-0",2,0,0,0);
	if(fd<0){out("I2C adapter open failed\n");return;}
	if(!sensor_validate(fd))goto close;
	if(operation==0){sensor_state_fd(fd);goto close;}
	if(operation==1){
		out("Applying exact139-pair retail SC1346 stream-off table\n");
		for(i=0;i<sizeof(sc1346_init_table)/sizeof(sc1346_init_table[0]);i++){
			result=sensor_write(fd,sc1346_init_table[i].reg,sc1346_init_table[i].value);
			if(result<0){out("Sensor init failed pair=0x");hex(i);out(" result=0x");hex(result);out("\n");goto close;}
			if(sc1346_init_table[i].reg==0x0103){long wait[2]={0,10000000};sc(162,(long)wait,0,0,0,0);}
		}
		out("SENSOR TABLE APPLIED; capture/frames not proved\n");sensor_state_fd(fd);
	}else{
		/* Stream control is explicit and reversible, never inferred from init. */
		result=sensor_write(fd,0x0100,operation==2?1:0);
		if(result<0){out("Sensor stream write failed\n");goto close;}
		result=sensor_read(fd,0x0100);
		out("SENSOR STREAM CONTROL READBACK 0x");hex(result);out("; not a captured frame\n");
	}
close:
	sc(6,fd,0,0,0,0);
}
static void sensor_id(void)
{
	unsigned char reg[2]={0x31,0x07},id[2]={0,0};
	struct i2c_msg_wire msgs[2];struct i2c_rdwr_wire request;long fd,result;u32 i;
	msgs[0].addr=0x30;msgs[0].flags=0;msgs[0].len=2;msgs[0].padding=0;msgs[0].buffer=reg;
	msgs[1].addr=0x30;msgs[1].flags=1;msgs[1].len=2;msgs[1].padding=0;msgs[1].buffer=id;
	request.messages=msgs;request.count=1;
	fd=sc(5,(long)"/dev/i2c-0",2,0,0,0);
	if(fd<0){out("I2C adapter open failed\n");return;}
	/* Match retail separate one-byte ID-register reads, with STOP between
	 * pointer and data phases rather than assuming repeated START/autoincrement.
	 */
	for(i=0;i<2;i++){
		reg[1]=0x07+i;request.messages=&msgs[0];
		result=sc(54,fd,0x0707,(long)&request,0,0);
		if(result!=1)break;
		msgs[1].len=1;msgs[1].buffer=&id[i];request.messages=&msgs[1];
		result=sc(54,fd,0x0707,(long)&request,0,0);
		if(result!=1)break;
	}
	sc(6,fd,0,0,0,0);
	if(result!=1){out("I2C fixed-ID read failed result=0x");hex(result);out("\n");return;}
	out("SC1346 fixed read silicon=0x");hex(((u32)id[0]<<8)|id[1]);out("\n");
}
#endif
static void cat(const char *path)
{
	char data[1024]; long fd, n; usize budget = 32768;
	if (!prefix(path, "/proc/") && !prefix(path, "/sys/")) { out("Only /proc and /sys reads allowed.\n"); return; }
	fd = sc(5, (long)path, 0, 0, 0, 0);
	if (fd < 0) { out("Open failed.\n"); return; }
	while (budget && (n = sc(3, fd, (long)data, sizeof(data), 0, 0)) > 0) {
		sc(4, 1, (long)data, n, 0, 0); budget = budget > (usize)n ? budget - n : 0;
	}
	sc(6, fd, 0, 0, 0, 0);
}
#ifdef QC07F_ISP_QUALIFICATION
static void isp_capture(void)
{
	long sensor,control,result,guard=-1;int timeout=5;char magic='V';
	unsigned char trigger[2]={'1','\n'};
	sensor=sc(5,(long)"/dev/i2c-0",2,0,0,0);
	if(sensor<0 || !sensor_validate(sensor))goto close_sensor;
	if(sensor_read(sensor,0x3208)!=5 || sensor_read(sensor,0x3209)!=0 ||
	   sensor_read(sensor,0x320a)!=2 || sensor_read(sensor,0x320b)!=0xd0){
		out("Refuse capture: initialize expected1280x720 sensor first\n");goto close_sensor;
	}
	control=sc(5,(long)"/sys/bus/platform/devices/20000000.isp/capture",1,0,0,0);
	if(control<0){out("ISP capture control unavailable\n");goto close_sensor;}
	if(!watchdog_stoppable()){out("Refuse ISP test without stoppable watchdog recovery\n");sc(6,control,0,0,0,0);goto close_sensor;}
	guard=sc(5,(long)"/dev/watchdog0",2,0,0,0);
	if(guard<0){out("Watchdog recovery open failed\n");sc(6,control,0,0,0,0);goto close_sensor;}
	result=sc(54,guard,0xc0045706,(long)&timeout,0,0);
	if(result<0 || timeout!=5){out("Watchdog recovery timeout setup failed\n");sc(6,control,0,0,0,0);goto close_sensor;}
	result=sensor_write(sensor,0x0100,1);
	if(!result){
		out("ISP bounded capture begins; sensor stream will be stopped afterward\n");
		result=sc(4,control,(long)trigger,2,0,0);
		out("ISP capture syscall result=0x");hex(result);out("\n");
	}
	sc(6,control,0,0,0,0);
	result=sensor_write(sensor,0x0100,0);
	if(result || sensor_read(sensor,0x0100)!=0)out("Sensor stop/readback failed\n");
	else out("Sensor stream stopped after capture\n");
	cat("/sys/bus/platform/devices/20000000.isp/state");
close_sensor:
	if(guard>=0){sc(4,guard,(long)&magic,1,0,0);sc(6,guard,0,0,0,0);}
	if(sensor>=0)sc(6,sensor,0,0,0,0);
}
static void frame_dump(int full)
{
	unsigned char bytes[64];char encoded[129];long fd,n;u32 i,offset=0;
	u32 limit=full?1728000:4096;
	fd=sc(5,(long)"/dev/qc07f-frame",0,0,0,0);
	if(fd<0){out("No captured frame device\n");return;}
	out("QCF_BEGIN\n");
	while(offset<limit){
		n=sc(3,fd,(long)bytes,sizeof(bytes),0,0);
		if(n<=0){out("QCF_READ_ERROR\n");break;}
		for(i=0;i<(u32)n;i++){
			encoded[2*i]="0123456789abcdef"[bytes[i]>>4];
			encoded[2*i+1]="0123456789abcdef"[bytes[i]&15];
		}
		encoded[2*n]=0;out("QCF ");hex(offset);out(" ");out(encoded);out("\n");offset+=n;
	}
	out("QCF_END ");hex(offset);out("\n");sc(6,fd,0,0,0,0);
}
static void frame_roi(int anchor)
{
	unsigned char bytes[64];char encoded[129];long fd,n;u32 row,block,i,bit,offset=0,crc=0xffffffff;
	fd=sc(5,(long)"/dev/qc07f-frame",0,0,0,0);
	if(fd<0){out("No captured frame device\n");return;}
	out(anchor?"QCROI_BEGIN x768 y448 width512 height256 LUMA\n":"QCROI_BEGIN x256 y448 width512 height256 LUMA\n");
	for(row=0;row<256;row++){
		if(sc(19,fd,(448+row)*1280+(anchor?768:256),0,0,0)<0)goto failure;
		for(block=0;block<8;block++){
			n=sc(3,fd,(long)bytes,sizeof(bytes),0,0);
			if(n!=64)goto failure;
			for(i=0;i<64;i++){
				encoded[2*i]="0123456789abcdef"[bytes[i]>>4];
				encoded[2*i+1]="0123456789abcdef"[bytes[i]&15];
				crc^=(u32)bytes[i]<<24;
				for(bit=0;bit<8;bit++)crc=(crc<<1)^((crc&0x80000000)?0x04c11db7:0);
			}
			encoded[128]=0;out("QCROI ");hex(offset);out(" ");out(encoded);out("\n");offset+=64;
		}
	}
	out("QCROI_END ");hex(offset);out(" CRC ");hex(crc^0xffffffff);out("\n");
	sc(6,fd,0,0,0,0);return;
failure:
	out("QCROI_READ_ERROR\n");sc(6,fd,0,0,0,0);
}
#endif
#ifdef QC07F_USB_QUALIFICATION
static void usb_startup_guard_stop(void)
{
	long fd;char magic='V';
	if(!watchdog_stoppable()){out("USB boot guard: stoppable watchdog unavailable\n");return;}
	fd=sc(5,(long)"/dev/watchdog0",2,0,0,0);
	if(fd<0){out("USB boot guard open failed\n");return;}
	sc(4,fd,(long)&magic,1,0,0);sc(6,fd,0,0,0,0);
	out("USB qualification startup watchdog magic-close completed\n");
}
static void usb_info(void)
{
	out("USB owned onboard device1-1 facts, if enumerated:\n");
	cat("/sys/bus/usb/devices/1-1/idVendor");
	cat("/sys/bus/usb/devices/1-1/idProduct");
	cat("/sys/bus/usb/devices/1-1/speed");
	cat("/sys/bus/usb/devices/1-1/bMaxPower");
	cat("/sys/bus/usb/devices/1-1/uevent");
}
struct usb_bulk_wire { u32 endpoint,length,timeout_ms; void *data; };
_Static_assert(sizeof(struct usb_bulk_wire)==16,"ARM32 USBDEVFS bulk UAPI");
static int sysfs_prefix_value(const char *path,const char *expected)
{
	char buffer[32];long fd,n;
	fd=sc(5,(long)path,0,0,0,0);if(fd<0)return 0;
	n=sc(3,fd,(long)buffer,sizeof(buffer)-1,0,0);sc(6,fd,0,0,0,0);
	if(n<0)return 0;buffer[n]=0;return prefix(buffer,expected);
}
static long radio_dev_open(void)
{
	char value[8],path[]="/dev/bus/usb/001/000";long fd,n;u32 i,number=0;volatile u32 tens=0;
	fd=sc(5,(long)"/sys/bus/usb/devices/1-1/devnum",0,0,0,0);if(fd<0)return fd;
	n=sc(3,fd,(long)value,sizeof(value),0,0);sc(6,fd,0,0,0,0);
	if(n<1)return -19;
	for(i=0;i<(u32)n && value[i]>='0' && value[i]<='9';i++)number=number*10+value[i]-'0';
	if(number<2 || number>127)return -19;
	if(number>=100){path[17]='1';number-=100;}
	while(number>=10){number-=10;tens++;}
	path[18]='0'+tens;path[19]='0'+number;
	return sc(5,(long)path,2,0,0,0);
}
static void radio_info(int boot_state)
{
	/* Retail REA get_fwinfo -> read-only command43; send_cmd exactly12 bytes,
	 * TX magic1a2b,type3,STA interface1,len12,cookie1. No firmware/OTA/eFuse,
	 * RF/channel/key/association command is emitted by this helper.
	 */
	unsigned char request[12]={0x2b,0x1a,3,1,12,0,1,0,43,0,0,0};
	/* Retail boot bus template checksum mode2, fragment2048; commandENTER
	 * identifies BOOT(0)/IN_FW(ff), no memory/register/flash write command.
	 */
	unsigned char boot_request[20]={0x2b,0x1a,7,0,20,0,2,0,0,12,2,'@','h','u','g','e','-','i','c',0xfd};
	unsigned char response[512];struct usb_bulk_wire bulk;u32 interface=0,i,attempt,cycle,cookie;
	long fd=-1,guard=-1,result;int timeout=5;char magic='V';int claimed=0;
	if(!sysfs_prefix_value("/sys/bus/usb/devices/1-1/idVendor","a012") ||
	   !sysfs_prefix_value("/sys/bus/usb/devices/1-1/idProduct","8400") ||
	   !sysfs_prefix_value("/sys/bus/usb/devices/1-1/bConfigurationValue","1")){
		out("Refuse radio query: expected owned configureda012:8400 not confirmed\n");return;
	}
	if(!watchdog_stoppable()){out("Refuse radio query without watchdog guard\n");return;}
	fd=radio_dev_open();if(fd<0){out("USB devfs open failed\n");return;}
	guard=sc(5,(long)"/dev/watchdog0",2,0,0,0);if(guard<0)goto cleanup;
	result=sc(54,guard,0xc0045706,(long)&timeout,0,0);if(result<0 || timeout!=5)goto cleanup;
	result=sc(54,fd,0x8004550f,(long)&interface,0,0); /* CLAIMINTERFACE */
	if(result<0){out("Radio interface claim failed\n");goto cleanup;}claimed=1;
	for(cycle=0;cycle<(boot_state>=2?4U:1U);cycle++){
	cookie=boot_state?2+cycle:1;boot_request[6]=cookie;
	if(boot_state!=2){
		u32 endpoint=1;
		result=sc(54,fd,0x80045515,(long)&endpoint,0,0); /* USBDEVFS_CLEAR_HALT */
		if(result<0){out("Radio OUT toggle reset failed\n");goto cleanup;}
		endpoint=0x81;result=sc(54,fd,0x80045515,(long)&endpoint,0,0);
		if(result<0){out("Radio IN toggle reset failed\n");goto cleanup;}
	}
	bulk.endpoint=1;bulk.length=boot_state?20:12;bulk.timeout_ms=500;bulk.data=boot_state?boot_request:request;
	result=sc(54,fd,0xc0105502,(long)&bulk,0,0);out(boot_state?"RADIO BOOT STATE QUERY TX RESULT 0x":"RADIO READONLY FWINFO TX RESULT 0x");hex(result);out("\n");
	if(result!=(boot_state?20:12))goto cleanup;
	for(attempt=0;attempt<4;attempt++){
		bulk.endpoint=0x81;bulk.length=sizeof(response);bulk.data=response;
		result=sc(54,fd,0xc0105502,(long)&bulk,0,0);
		out("RADIO RX RESULT 0x");hex(result);out("\n");if(result<0)goto cleanup;
		out("RADIO RX BYTES ");for(i=0;i<(u32)result;i++){char pair[3];pair[0]="0123456789abcdef"[response[i]>>4];pair[1]="0123456789abcdef"[response[i]&15];pair[2]=0;out(pair);}out("\n");
		if(boot_state && result>=16 && response[0]==0x1a && response[1]==0x2b && response[6]==cookie && response[7]==0 && response[8]==0 && response[15]==0xfd){
			out("RADIO BOOT QUERY CHECK/CMD/COOKIE MATCH, RESULT 0x");hex(response[9]);out("\n");
			if(response[9]==0)out("RADIO STATE BOOTLOADER\n");
			else if(response[9]==0xff)out("RADIO STATE APPLICATION\n");
			else out("RADIO STATE UNRESOLVED, no download authorized by this result\n");
			break;
		}
		if(!boot_state && result>=12 && response[0]==0x1a && response[1]==0x2b && response[2]==3 && response[6]==1 && response[7]==0 && response[8]==43){
			out("RADIO FWINFO COOKIE/CMD MATCH, RETURN/PAYLOAD LENGTH 0x");hex((u32)response[10]|((u32)response[11]<<8));out("\n");break;
		}
	}
	if(attempt==4)goto cleanup;
	{char ping='.';sc(4,guard,(long)&ping,1,0,0);}
	}
cleanup:
	if(claimed)sc(54,fd,0x80045510,(long)&interface,0,0);
	if(guard>=0){sc(4,guard,(long)&magic,1,0,0);sc(6,guard,0,0,0,0);}
	if(fd>=0)sc(6,fd,0,0,0,0);
}
#endif
#ifdef QC07F_RADIO_RAM_LOADER
#include "radio-ram-loader.h"
#endif
#ifdef QC07F_WIFI_QUALIFICATION
#include "wifi-monitor.h"
#endif
#ifdef QC07F_PTZ_QUALIFICATION
static void ptz_pulse(u32 axis,int forward)
{
	const char *pulse=axis?"/sys/bus/platform/devices/soc:ptz-axis1/pulse":"/sys/bus/platform/devices/soc:ptz-axis0/pulse";
	const char *state=axis?"/sys/bus/platform/devices/soc:ptz-axis1/state":"/sys/bus/platform/devices/soc:ptz-axis0/state";
	long fd=-1,guard=-1,result;int timeout=5;char magic='V';
	const char *command=forward?"4\n":"-4\n";
	if(!watchdog_stoppable()){out("Refuse motor test without stoppable watchdog\n");return;}
	fd=sc(5,(long)pulse,1,0,0,0);
	if(fd<0){out("Motor diagnostic unavailable\n");return;}
	guard=sc(5,(long)"/dev/watchdog0",2,0,0,0);
	if(guard<0){out("Motor watchdog guard unavailable\n");goto cleanup;}
	result=sc(54,guard,0xc0045706,(long)&timeout,0,0);
	if(result<0 || timeout!=5){out("Motor watchdog guard timeout failed\n");goto cleanup;}
	out("Explicit4-step motor pulse,20ms each; orientation/position unknown\n");
	result=sc(4,fd,(long)command,length(command),0,0);
	out("MOTOR PULSE SYSCALL RESULT 0x");hex(result);out("\n");cat(state);
cleanup:
	if(guard>=0){sc(4,guard,(long)&magic,1,0,0);sc(6,guard,0,0,0,0);}
	if(fd>=0)sc(6,fd,0,0,0,0);
}
#endif
static void help(void)
{
	#if defined(QC07F_PERIPHERAL_QUALIFICATION) || defined(QC07F_I2C_QUALIFICATION)
	out("QC07F experimental mainline qualification monitor\n"
	    "help | uptime | mem | irqs | cpu | cmdline | iomem | cat /proc/FILE\n"
	    "No flash/persistent writes; explicit hardware qualification operations below.\n");
	#else
	out("QC07F experimental mainline read-only monitor\n"
	    "help | uptime | mem | irqs | cpu | cmdline | iomem | cat /proc/FILE\n"
	    "No flash, GPIO, PTZ, sensor or persistent-configuration writes.\n");
	#endif
#ifdef QC07F_PERIPHERAL_QUALIFICATION
	out("gpio info: read-only chip/line inspection (no requests or values).\n"
	    "wdt smoke: explicit5s timeout/ping/magic-close register test.\n"
	    "wdt reset: explicit nominal1s reset; arm recovery capture first.\n");
#endif
#ifdef QC07F_I2C_QUALIFICATION
	out("sensor id: fixed address0x30 registers3107/3108; no sensor value writes or scans.\n");
	out("sensor state | sensor init (exact139-pair table) | sensor start | sensor stop\n");
#endif
#ifdef QC07F_ISP_QUALIFICATION
	out("isp capture (bounded2s DMA test) | isp state | frame head | frame dump\n");
	out("frame roi: exact512x256 luma crop x256/y448, with transfer CRC\n");
	out("frame anchor: exact512x256 luma crop x768/y448, with transfer CRC\n");
#endif
#ifdef QC07F_USB_QUALIFICATION
	out("usb info: onboard device descriptors/sysfs only; no radio firmware/network credentials\n");
	out("radio info: guarded read-only firmware-info command43; no downloads/association\n");
	out("radio state: retail keyed boot-state query only; no downloads/flash/OTP\n");
	out("radio repeat: four sequenced state queries to qualify repeated bulk exchange\n");
	out("radio resetrepeat: same queries with standard endpoint-toggle reset before each\n");
#endif
#ifdef QC07F_RADIO_RAM_LOADER
	out("radio load: exact chip8400 retail RAM-only load, state/security/CRC guarded; no SPI/OTA/OTP\n");
	out("radio native-load: same image/guards,2048-byte fragments without endpoint resets; tests corrected HCD\n");
#endif
#ifdef QC07F_PTZ_QUALIFICATION
	out("ptz0 + | ptz0 - | ptz1 + | ptz1 - : bounded4-step pulse,5s guard; no homing\n");
#endif
#ifdef QC07F_WIFI_QUALIFICATION
	out("wifi attach: explicit audited station-only module; verify RAM firmware first\n");
	out("shell: RAM-only BusyBox shell; exit returns to monitor; no credentials in built image\n");
#endif
}
void init_main(void)
{
	char line[256], ch; usize n = 0; long fd, result;
	long pause[2] = {1, 0};
	sc(21, (long)"devtmpfs", (long)"/dev", (long)"devtmpfs", 0, 0);
	sc(21, (long)"proc", (long)"/proc", (long)"proc", 0, 0);
	sc(21, (long)"sysfs", (long)"/sys", (long)"sysfs", 0, 0);
	fd = sc(5, (long)"/dev/console", 2, 0, 0, 0);
	if (fd < 0) fd = sc(5, (long)"/dev/ttyAK0", 2, 0, 0, 0);
	if (fd >= 0) { sc(63, fd, 0, 0, 0, 0); sc(63, fd, 1, 0, 0, 0); sc(63, fd, 2, 0, 0, 0); }
	else { for (;;) sc(162, (long)pause, 0, 0, 0, 0); }
#ifdef QC07F_USB_QUALIFICATION
	usb_startup_guard_stop();
#endif
	help(); out("qc07f> ");
	for (;;) {
		result = sc(3, 0, (long)&ch, 1, 0, 0);
		if (result <= 0) { sc(162, (long)pause, 0, 0, 0, 0); continue; }
		if (ch != '\n' && ch != '\r') { if (n + 1 < sizeof(line)) line[n++] = ch; continue; }
		line[n] = 0;
		if (equal(line, "help")) help();
		else if (equal(line, "uptime")) cat("/proc/uptime");
		else if (equal(line, "mem")) cat("/proc/meminfo");
		else if (equal(line, "irqs")) cat("/proc/interrupts");
		else if (equal(line, "cpu")) cat("/proc/cpuinfo");
		else if (equal(line, "cmdline")) cat("/proc/cmdline");
		else if (equal(line, "iomem")) cat("/proc/iomem");
#ifdef QC07F_PERIPHERAL_QUALIFICATION
		else if (equal(line,"gpio info")) gpio_info();
		else if (equal(line,"wdt smoke")) watchdog_test(0);
		else if (equal(line,"wdt reset")) watchdog_test(1);
#endif
		#ifdef QC07F_I2C_QUALIFICATION
		else if (equal(line,"sensor id")) sensor_id();
		else if (equal(line,"sensor state")) sensor_control(0);
		else if (equal(line,"sensor init")) sensor_control(1);
		else if (equal(line,"sensor start")) sensor_control(2);
		else if (equal(line,"sensor stop")) sensor_control(3);
		#endif
		#ifdef QC07F_ISP_QUALIFICATION
		else if (equal(line,"isp capture")) isp_capture();
		else if (equal(line,"isp state")) cat("/sys/bus/platform/devices/20000000.isp/state");
		else if (equal(line,"frame head")) frame_dump(0);
		else if (equal(line,"frame dump")) frame_dump(1);
		else if (equal(line,"frame roi")) frame_roi(0);
		else if (equal(line,"frame anchor")) frame_roi(1);
		#endif
		#ifdef QC07F_USB_QUALIFICATION
		else if (equal(line,"usb info")) usb_info();
		else if (equal(line,"radio info")) radio_info(0);
		else if (equal(line,"radio state")) radio_info(1);
		else if (equal(line,"radio repeat")) radio_info(2);
		else if (equal(line,"radio resetrepeat")) radio_info(3);
		#endif
		#ifdef QC07F_RADIO_RAM_LOADER
		else if (equal(line,"radio load")) radio_load(0);
		else if (equal(line,"radio native-load")) radio_load(1);
		#endif
		#ifdef QC07F_WIFI_QUALIFICATION
		else if (equal(line,"wifi attach")) wifi_attach();
		else if (equal(line,"shell")) wifi_shell();
		#endif
		#ifdef QC07F_PTZ_QUALIFICATION
		else if (equal(line,"ptz0 +")) ptz_pulse(0,1);
		else if (equal(line,"ptz0 -")) ptz_pulse(0,0);
		else if (equal(line,"ptz1 +")) ptz_pulse(1,1);
		else if (equal(line,"ptz1 -")) ptz_pulse(1,0);
		#endif
		else if (prefix(line, "cat ")) cat(line + 4);
		else if (n) out("Unknown command; use help.\n");
		n = 0; out("qc07f> ");
	}
}
__asm__(".global _start\n.type _start,%function\n_start:\nbl init_main\n1: b 1b\n");

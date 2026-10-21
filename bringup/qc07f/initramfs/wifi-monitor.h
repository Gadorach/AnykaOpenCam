/* SPDX-License-Identifier: GPL-2.0-only */
static void wifi_attach(void)
{
	long fd=-1,guard=-1,result;int timeout=5;char magic='V';
	long settle[2]={2,0};
	if(!sysfs_prefix_value("/sys/bus/usb/devices/1-1/idVendor","a012") ||
	   !sysfs_prefix_value("/sys/bus/usb/devices/1-1/idProduct","8400") ||
	   !sysfs_prefix_value("/sys/bus/usb/devices/1-1/bConfigurationValue","1") ||
	   !watchdog_stoppable()){
		out("Refuse WiFi attach: owned configuration/guard unconfirmed\n");return;
	}
	fd=sc(5,(long)"/hgics.ko",0,0,0,0);if(fd<0){out("Audited module missing\n");return;}
	guard=sc(5,(long)"/dev/watchdog0",2,0,0,0);if(guard<0)goto cleanup;
	result=sc(54,guard,0xc0045706,(long)&timeout,0,0);if(result<0 || timeout!=5)goto cleanup;
	out("WiFi station-only driver attach; automatic boot downloads/OTA/maintenance denied\n");
	result=sc(379,fd,(long)"no_bootdl=1",0,0,0);
	out("WiFi FINIT_MODULE RESULT0x");hex(result);out("\n");
	if(result>=0){sc(162,(long)settle,0,0,0,0);cat("/sys/class/net/wlan0/operstate");}
cleanup:
	if(guard>=0){sc(4,guard,(long)&magic,1,0,0);sc(6,guard,0,0,0,0);}
	if(fd>=0)sc(6,fd,0,0,0,0);
}
static void wifi_shell(void)
{
	long pid=sc(2,0,0,0,0,0),status=0;
	if(pid==0){
		const char *argv[]={"/bin/busybox","ash",0};
		const char *env[]={"PATH=/bin","HOME=/","TERM=vt100",0};
		sc(66,0,0,0,0,0); /* setsid; no reset/boot control wiring */
		sc(54,0,0x540e,0,0,0); /* TIOCSCTTY */
		sc(11,(long)argv[0],(long)argv,(long)env,0,0);
		sc(1,127,0,0,0,0);
	}
	if(pid<0){out("Shell fork failed\n");return;}
	while(sc(114,pid,(long)&status,0,0,0)==-4){}
	out("RAM-only shell returned to monitor\n");
}

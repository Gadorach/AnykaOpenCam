#!/usr/bin/env python3
"""Pinned mainline UART-first build; does not boot, deploy or flash a target."""
import argparse,hashlib,json,os,shutil,struct,subprocess,zlib
from pathlib import Path

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--session',required=True);parser.add_argument('--resume',action='store_true');parser.add_argument('--attempt',default='2');parser.add_argument('--stock-ram',action='store_true',help='Use measured QC07F40MiB System RAM and fixed81808000 decompression target');parser.add_argument('--qualify',action='store_true',help='Explicit GPIO metadata and watchdog qualification');parser.add_argument('--i2c',action='store_true',help='Enable bounded I2C bootstrap controller and fixed SC1346 tests');parser.add_argument('--sensor',action='store_true',help='Source-supported24MHz sensor MCLK and bounded reset41 pulse');parser.add_argument('--isp',action='store_true',help='Explicit bounded first-frame ISP/DMA diagnostic, not production capture');parser.add_argument('--usb',action='store_true',help='Upstream indexed MUSB host PIO bootstrap');parser.add_argument('--ptz',action='store_true',help='Explicit board5 bounded4-step motor diagnostics, no probe actuation');parser.add_argument('--radio-loader',action='store_true',help='Explicit same-retail-image radio RAM loader; never auto-run or flash');args=parser.parse_args()
    if args.i2c and not (args.stock_ram and args.qualify):raise SystemExit('I2C tests require stock RAM and qualification mode')
    if args.sensor and not args.i2c:raise SystemExit('Sensor bootstrap requires I2C qualification mode')
    if args.isp and not args.sensor:raise SystemExit('ISP diagnostic requires sensor qualification mode')
    if args.usb and not (args.stock_ram and args.qualify and args.isp):raise SystemExit('USB qualification currently requires full measured-RAM peripheral image')
    if args.ptz and not args.usb:raise SystemExit('Current PTZ profile requires full guarded peripheral image')
    if args.radio_loader and not args.usb:raise SystemExit('Radio RAM loader requires USB profile')
    session=Path(args.session).resolve();kernel=session/'sources/linux-lts';output=session/'build/linux-lts'
    project=Path(__file__).resolve().parents[2];overlay=Path(__file__).resolve().parent
    import sys
    sys.path.insert(0,str(session));from run_logged import run as run_command,now
    def run(label,argv,**kw):
        return run_command((('attempt'+args.attempt+'-') if args.resume else '')+label,[str(v) for v in argv],**kw)
    # Source import into a dedicated pinned 6.18 LTS checkout, never original dumps.
    makefile=kernel/'arch/arm/Makefile';kconfig=kernel/'arch/arm/Kconfig.platforms'
    addition='machine-$(CONFIG_ARCH_AK3918EV200) += ak3918ev200\n'
    fixmap=kernel/'arch/arm/include/asm/fixmap.h'
    if 'FIX_AK39E2_L2CTRL' not in fixmap.read_text():
        fixmap.write_text(fixmap.read_text().replace('\tFIX_EARLYCON_MEM_BASE,','\tFIX_EARLYCON_MEM_BASE,\n#ifdef CONFIG_ARCH_AK3918EV200\n\tFIX_AK39E2_L2CTRL,\n\tFIX_AK39E2_UARTBUF,\n#endif'))
    old_addition='obj-$(CONFIG_ARCH_AK3918EV200) += mach-ak3918ev200/\n'
    machine_text=makefile.read_text()
    if old_addition in machine_text:
        machine_text=machine_text.replace(old_addition,'')
    if addition not in machine_text:
        anchor='# legacy platforms provide their own mach/*.h headers globally,'
        if machine_text.count(anchor)!=1:raise SystemExit('Unexpected ARM machine-y anchor')
        machine_text=machine_text.replace(anchor,addition+'\n'+anchor)
    if machine_text!=makefile.read_text():makefile.write_text(machine_text)
    if 'config ARCH_AK3918EV200' not in kconfig.read_text():kconfig.write_text(kconfig.read_text()+'''\nconfig ARCH_AK3918EV200
\tbool "Anyka AK3918EV200 experimental QC07F bring-up"
\tdepends on ARCH_MULTI_V5
\tselect IRQ_DOMAIN
\tselect TIMER_OF
\tselect COMMON_CLK
\tselect SERIAL_CORE
\tselect SERIAL_CORE_CONSOLE
\tselect SERIAL_EARLYCON
\thelp
\t  Offline-built prototype only. Register definitions are from the legacy
\t  BSP; retail clock/RAM/pin state and actual boot remain unverified.
''')
    if 'config GPIO_AK3918EV200' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig GPIO_AK3918EV200
\tbool "Anyka AK3918EV200 retail-register GPIO first port"
\tdepends on ARCH_AK3918EV200 && GPIOLIB
\tselect MFD_SYSCON
\thelp
\t  Retail REA-correlated GPIO direction/value/pull register port.
\t  Leaves all board outputs and mux untouched at probe; GPIO49 is
\t  input-only. GPIO IRQ/pinctrl ownership integration is not yet complete.
\t  The prototype DTS keeps the node disabled pending qualification.
''')
    if 'config WATCHDOG_AK3918EV200' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig WATCHDOG_AK3918EV200
\tbool "Anyka AK3918EV200 keyed TOP watchdog first port"
\tdepends on ARCH_AK3918EV200 && WATCHDOG
\tselect WATCHDOG_CORE
\thelp
\t  Retail REA-correlated keyed TOP watchdog using watchdog core.
\t  Probe does not write hardware; DT stays disabled until timeout,
\t  keepalive, stop/nowayout and reset qualification on the target.
''')
    if 'config I2C_AK3918EV200' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig I2C_AK3918EV200
\tbool "Anyka AK3918EV200 bounded PIO I2C bootstrap port"
\tdepends on ARCH_AK3918EV200 && I2C
\tselect MFD_SYSCON
\thelp
\t  Retail REA-correlated controller with50ms completion polling.
\t  Bootstrap ASIC clock decoding/reset/pin27/28 mux is explicit.
\t  Production CCF/reset/pinctrl and NACK semantics remain to qualify.
''')
    if 'config SENSOR_BOOTSTRAP_QC07F' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig SENSOR_BOOTSTRAP_QC07F
\tbool "QC07F sensor clock/reset bootstrap"
\tdepends on ARCH_AK3918EV200 && GPIOLIB
\tselect MFD_SYSCON
\thelp
\t  Retail-metadata reset41 and recovered24MHz sensor MCLK sequence.
\t  No PLL retuning, sensor-value write, motor or expander command.
\t  Enabled by explicit sensor qualification build mode only.
''')
    if 'config ISP_DIAGNOSTIC_QC07F' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig ISP_DIAGNOSTIC_QC07F
\tbool "QC07F bounded ISP first-frame diagnostic"
\tdepends on ARCH_AK3918EV200
\tselect MFD_SYSCON
\thelp
\t  Polled bounded capture into owned coherent DMA buffers with guards.
\t  Sanitized live retail register/calibration facts, no executable blob.
\t  Parent IRQ stays masked; no capture at probe. Not production V4L2.
''')
    if 'config USB_AK3918EV200' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig USB_AK3918EV200
\tbool "Anyka AK3918EV200 indexed MUSB PIO host bootstrap"
\tdepends on ARCH_AK3918EV200 && USB && USB_MUSB_HDRC && NOP_USB_XCEIV
\tselect MFD_SYSCON
\thelp
\t  Source-supported clock/reset15 and PHY role/power setup with upstream
\t  indexed MUSB PIO scheduler. No L2 DMA, radio firmware or GPIO action.
\t  Register compatibility/IRQ/retail USB enumeration require qualification.
''')
    if 'config PTZ_DIAGNOSTIC_QC07F' not in kconfig.read_text():
        kconfig.write_text(kconfig.read_text()+'''\nconfig PTZ_DIAGNOSTIC_QC07F
\tbool "QC07F board5 bounded motor diagnostics"
\tdepends on ARCH_AK3918EV200 && GPIOLIB
\thelp
\t  Exact retail03/06/0c/09 phase sequence on described direct GPIOs.
\t  No requests/writes at probe; explicit capped4-step commands only.
\t  Mechanical axis orientation/position/limits remain unqualified.
''')
    src=overlay/'mainline/arch/arm/mach-ak3918ev200';dest=kernel/'arch/arm/mach-ak3918ev200'
    if not dest.exists():shutil.copytree(src,dest)
    else:
        for file in src.iterdir():
            target=dest/file.name
            if not target.exists():
                with target.open('xb') as f:f.write(file.read_bytes())
            elif target.read_bytes()!=file.read_bytes():raise SystemExit('Existing kernel overlay differs; preserve it and apply a recorded local patch.')
    header=overlay/'mainline/arch/arm/include/debug/ak3918ev200-uncompress.h'
    target=kernel/'arch/arm/include/debug/ak3918ev200-uncompress.h'
    if target.exists():
        if target.read_bytes()!=header.read_bytes():raise SystemExit('Existing decompressor header differs; preserve source.')
    else:
        with target.open('xb') as f:f.write(header.read_bytes())
    debug=kernel/'arch/arm/Kconfig.debug'
    selected='\tdefault "debug/ak3918ev200-uncompress.h" if ARCH_AK3918EV200\n'
    if selected not in debug.read_text():
        debug.write_text(debug.read_text().replace('config UNCOMPRESS_INCLUDE\n\tstring\n','config UNCOMPRESS_INCLUDE\n\tstring\n'+selected))
    if args.stock_ram:
        # AUTO_ZRELADDR is a hidden default for multiplatform. Appended DTB
        # skips memory-start validation and masks PC down to80000000, which
        # contradicts measured stock81800000 System RAM. Use PHYS_OFFSET.
        armconfig=kernel/'arch/arm/Kconfig'
        default='\tdefault n if ARCH_AK3918EV200\n'
        if default not in armconfig.read_text():
            armconfig.write_text(armconfig.read_text().replace('config AUTO_ZRELADDR\n','config AUTO_ZRELADDR\n'+default))
    # Explicit branch prevents fall-through into linker-created alignment
    # filler between .start and .text. RAM disassembly found0xd4d4d4d4 here.
    head=kernel/'arch/arm/boot/compressed/head.S'
    marker='\t\tb\t.Lqc07f_text_start\n'
    if marker not in head.read_text():
        head.write_text(head.read_text().replace('\t\t.text\n\n#ifdef CONFIG_AUTO_ZRELADDR','''#ifdef CONFIG_ARCH_AK3918EV200
\t\tb\t.Lqc07f_text_start
#endif
\t\t.text
.Lqc07f_text_start:

#ifdef CONFIG_AUTO_ZRELADDR'''))
    kernel_head=kernel/'arch/arm/kernel/head.S'
    if 'QC07F bootstrap UART mappings' not in kernel_head.read_text():
        anchor='#ifdef CONFIG_DEBUG_LL\n#if !defined(CONFIG_DEBUG_ICEDCC) && !defined(CONFIG_DEBUG_SEMIHOSTING)'
        mappings='''#ifdef CONFIG_ARCH_AK3918EV200
\t/* QC07F bootstrap UART mappings, retained through machine map_io.
\t * One controller section covers UART0 and L2; one SRAM section.
\t * Selected CPU IO flags, no peripheral initialization or GPIO writes.
\t */
\tldr\tr7, [r10, #PROCINFO_IO_MMUFLAGS]
\tldr\tr0, =(0xfef00000 >> (SECTION_SHIFT - PMD_ENTRY_ORDER))
\tadd\tr0, r4, r0
\tldr\tr3, =0x20100000
\torr\tr3, r3, r7
\tstr\tr3, [r0]
\tldr\tr0, =(0xfee00000 >> (SECTION_SHIFT - PMD_ENTRY_ORDER))
\tadd\tr0, r4, r0
\tldr\tr3, =0x48000000
\torr\tr3, r3, r7
\tstr\tr3, [r0]
#endif

'''
        if kernel_head.read_text().count(anchor)!=1:raise SystemExit('Unexpected ARM head mapping anchor')
        kernel_head.write_text(kernel_head.read_text().replace(anchor,mappings+anchor))
    main=kernel/'init/main.c'
    if 'ak39e2_boot_marker' not in main.read_text():
        source=main.read_text()
        declaration='''#ifdef CONFIG_ARCH_AK3918EV200
extern void ak39e2_boot_marker(unsigned int stage);
#define QC07F_BOOT_STAGE(n) ak39e2_boot_marker(n)
#else
#define QC07F_BOOT_STAGE(n) do { } while (0)
#endif

'''
        anchor='asmlinkage __visible __init __no_sanitize_address __noreturn __no_stack_protector\nvoid start_kernel(void)'
        if source.count(anchor)!=1:raise SystemExit('Unexpected start_kernel marker anchor')
        source=source.replace(anchor,declaration+anchor)
        substitutions=[
            ('\tset_task_stack_end_magic(&init_task);','\tQC07F_BOOT_STAGE(1);\n\tset_task_stack_end_magic(&init_task);'),
            ('\tcgroup_init_early();','\tQC07F_BOOT_STAGE(2);\n\tcgroup_init_early();'),
            ('\tsetup_arch(&command_line);','\tQC07F_BOOT_STAGE(3);\n\tsetup_arch(&command_line);\n\tQC07F_BOOT_STAGE(4);'),
            ('\tinit_IRQ();','\tQC07F_BOOT_STAGE(5);\n\tinit_IRQ();'),
            ('\ttime_init();','\ttime_init();\n\tQC07F_BOOT_STAGE(6);'),
            ('\tcalibrate_delay();','\tQC07F_BOOT_STAGE(7);\n\tcalibrate_delay();\n\tQC07F_BOOT_STAGE(8);'),
        ]
        for old,new in substitutions:
            if source.count(old)!=1:raise SystemExit('Unexpected boot-stage callsite '+old)
            source=source.replace(old,new)
        main.write_text(source)
    output.mkdir(exist_ok=args.resume)
    init=session/'artifacts'/('init-monitor-attempt'+args.attempt if args.resume and args.attempt!='2' and args.attempt!='3' else 'init-monitor')
    cmd=['clang','--target=arm-linux-gnueabi','-march=armv5te','-mcpu=arm926ej-s','-marm','-mfloat-abi=soft','-Oz',
         '-ffreestanding','-fno-builtin','-fno-stack-protector','-nostdlib','-static','-fuse-ld=lld','-Wl,-e,_start','-Wl,--build-id=none',
         '-o',str(init),str(overlay/'initramfs/init.c')]
    if args.qualify:cmd.insert(1,'-DQC07F_PERIPHERAL_QUALIFICATION=1')
    if args.i2c:cmd.insert(1,'-DQC07F_I2C_QUALIFICATION=1')
    if args.isp:cmd.insert(1,'-DQC07F_ISP_QUALIFICATION=1')
    if args.usb:cmd.insert(1,'-DQC07F_USB_QUALIFICATION=1')
    if args.ptz:cmd.insert(1,'-DQC07F_PTZ_QUALIFICATION=1')
    if args.radio_loader:cmd.insert(1,'-DQC07F_RADIO_RAM_LOADER=1')
    if not args.resume or not init.exists():
        payload_inputs=[session/'artifacts/retail-radio-payload36.bin'] if args.radio_loader else []
        if run('build-init-monitor',cmd,inputs=[overlay/'initramfs/init.c',*sorted((overlay/'initramfs').glob('*.h')),*payload_inputs],outputs=[init]):raise SystemExit('Monitor compile failed')
    init.chmod(0o444)
    # Deterministic newc archive, including device-node metadata without host mknod.
    archive=bytearray();ino=1
    def member(name,mode,data=b'',rmajor=0,rminor=0):
        nonlocal ino
        encoded=name.encode()+b'\0'
        fields=[ino,mode,0,0,1,0,len(data),0,0,rmajor,rminor,len(encoded),0];ino+=1
        archive.extend(b'070701'+b''.join(f'{v:08x}'.encode() for v in fields));archive.extend(encoded)
        archive.extend(bytes((-len(archive))%4));archive.extend(data);archive.extend(bytes((-len(archive))%4))
    for name in ['dev','proc','sys']:member(name,0o040755)
    member('dev/console',0o020600,rmajor=5,rminor=1)
    member('init',0o100755,init.read_bytes());member('TRAILER!!!',0)
    cpio=session/'artifacts'/('initramfs-attempt'+args.attempt+'.cpio' if args.resume and args.attempt!='2' and args.attempt!='3' else 'initramfs.cpio')
    if cpio.exists():
        if not args.resume or cpio.read_bytes()!=archive:raise SystemExit('Existing initramfs differs; refusing overwrite')
    else:
        with cpio.open('xb') as f:f.write(archive)
    env={**os.environ,'KBUILD_BUILD_TIMESTAMP':'2026-10-08 00:00:00 UTC','KBUILD_BUILD_USER':'qc07f','KBUILD_BUILD_HOST':'offline-bringup'}
    base=['make','-C',str(kernel),'O='+str(output),'ARCH=arm','LLVM=1']
    # Generic allnoconfig, then explicitly choose ARMv5 + only this machine.
    if run('kernel-allnoconfig',base+['allnoconfig']):raise SystemExit('allnoconfig failed')
    cfg=output/'.config';config=kernel/'scripts/config'
    flags=['--file',str(cfg)]
    for name in ['ARCH_MULTI_V5','ARCH_AK3918EV200','CPU_ARM926T','OF','USE_OF','MMU','BINFMT_ELF','BLK_DEV_INITRD','DEVTMPFS',
                 'DEVTMPFS_MOUNT','PROC_FS','SYSFS','TMPFS','PRINTK','TTY','VT','UNIX98_PTYS','SERIAL_CORE','SERIAL_CORE_CONSOLE',
                  'SERIAL_EARLYCON','ARM_ATAG_DTB_COMPAT','ARM_ATAG_DTB_COMPAT_CMDLINE_FROM_BOOTLOADER','ARM_APPENDED_DTB','IKCONFIG','IKCONFIG_PROC','KALLSYMS',
                   'GPIOLIB','GPIO_CDEV','GPIO_AK3918EV200','MFD_SYSCON','WATCHDOG','WATCHDOG_AK3918EV200','WATCHDOG_SYSFS','I2C','I2C_CHARDEV','I2C_AK3918EV200','SENSOR_BOOTSTRAP_QC07F','ISP_DIAGNOSTIC_QC07F']:
        flags+=['--enable',name]
    if args.usb:
        for name in ['USB_SUPPORT','USB','USB_MUSB_HDRC','USB_MUSB_HOST','MUSB_PIO_ONLY','USB_AK3918EV200','NOP_USB_XCEIV','EXPERT','USB_OTG_DISABLE_EXTERNAL_HUB']:
            flags+=['--enable',name]
    if args.ptz:flags+=['--enable','PTZ_DIAGNOSTIC_QC07F']
    for name in ['ARCH_MULTI_V7','ARCH_MULTI_V6','SMP','MODULES','NET','MMC','MTD','COMPILE_TEST','RUST','DEBUG_INFO','STACKPROTECTOR','KERNEL_GZIP']+([] if args.usb else ['USB']):
        flags+=['--disable',name]
    flags+=['--enable','KERNEL_XZ']
    flags+=['--set-str','INITRAMFS_SOURCE',str(cpio),'--set-str','LOCALVERSION','-qc07f-prototype',
            '--set-val','PHYS_OFFSET','0x81800000' if args.stock_ram else '0x80000000','--set-val','HZ',100]
    if run('kernel-config', [str(config),*flags],inputs=[cfg]):raise SystemExit('config failed')
    if run('kernel-olddefconfig',base+['olddefconfig']):raise SystemExit('olddefconfig failed')
    # Enforce the intended architecture rather than accepting silently dropped options.
    text=cfg.read_text()
    if args.usb:
        for option in ['USB','USB_MUSB_HDRC','USB_MUSB_HOST','MUSB_PIO_ONLY','USB_AK3918EV200','NOP_USB_XCEIV']:
            if f'CONFIG_{option}=y' not in text:raise SystemExit('Missing USB host qualification config '+option)
        if 'CONFIG_USB_INVENTRA_DMA=y' in text or 'CONFIG_USB_MUSB_DUAL_ROLE=y' in text:raise SystemExit('Unexpected USB DMA/dual-role enablement')
    for name in ['ARCH_AK3918EV200','CPU_ARM926T','ARCH_MULTI_V5']:
        if f'CONFIG_{name}=y' not in text:raise SystemExit('Missing required config '+name)
    if args.stock_ram and ('CONFIG_AUTO_ZRELADDR=y' in text or 'CONFIG_PHYS_OFFSET=0x81800000' not in text):
        raise SystemExit('Stock RAM/decompression configuration did not take effect')
    dtb=session/'artifacts/qc07f-prototype.dtb'
    if dtb.exists():
        dtb=session/'artifacts'/('qc07f-prototype-attempt'+args.attempt+'.dtb')
    dts=overlay/'qc07f-prototype.dts'
    if args.stock_ram:
        dts=session/'artifacts'/('qc07f-stock-window-attempt'+args.attempt+'.dts')
        stock_text=(overlay/'qc07f-prototype.dts').read_text().replace('memory@80000000','memory@81800000').replace('reg = <0x80000000 0x04000000>;','reg = <0x81800000 0x02800000>;')
        stock_text=stock_text.replace(' * peripherals-disabled prototype does not use that ABI; its full-DRAM layout\n * still needs a safe loading/decompression experiment, not an assumed address.',
                                     ' * generated stock-window variant uses the measured40MiB System RAM.\n * Decompression is fixed at81808000; input must be staged in a disjoint range.')
        if args.qualify:
            # Exactly the two existing disabled nodes, with no hogs/consumers.
            if stock_text.count('status = "disabled";')!=8:raise SystemExit('Unexpected qualification node count')
            stock_text=stock_text.replace('status = "disabled";','status = "okay";',2)
        if args.i2c:
            if stock_text.count('status = "disabled";')!=6:raise SystemExit('Unexpected I2C qualification node count')
            stock_text=stock_text.replace('status = "disabled";','status = "okay";',1)
        if args.sensor:
            if stock_text.count('status = "disabled";')!=5:raise SystemExit('Unexpected sensor qualification node count')
            stock_text=stock_text.replace('status = "disabled";','status = "okay";',1)
        if args.isp:
            if stock_text.count('status = "disabled";')!=4:raise SystemExit('Unexpected ISP qualification node count')
            stock_text=stock_text.replace('status = "disabled";','status = "okay";',1)
        if args.usb:
            if stock_text.count('status = "disabled";')!=3:raise SystemExit('Unexpected USB qualification node count')
            stock_text=stock_text.replace('status = "disabled";','status = "okay";',1)
        if args.ptz:
            if stock_text.count('status = "disabled";')!=2:raise SystemExit('Unexpected PTZ qualification node count')
            stock_text=stock_text.replace('status = "disabled";','status = "okay";')
        with dts.open('x') as f:f.write(stock_text)
    if run('prototype-dtc',['dtc','-I','dts','-O','dtb','-o',str(dtb),str(dts)],
           inputs=[dts],outputs=[dtb]):raise SystemExit('DTC failed')
    # run_logged inherits environment; only reproducible build identity is altered.
    os.environ.update({k:env[k] for k in ['KBUILD_BUILD_TIMESTAMP','KBUILD_BUILD_USER','KBUILD_BUILD_HOST']})
    if run('mainline-kernel-build',base+['-j4','zImage']):raise SystemExit('Kernel build failed; retain log/source/output')
    # Config selection/build success does not prove the platform was linked.
    # ARM architecture Makefile uses machine-y/core-y, not obj-y at file end.
    linkage=[]
    for unit in ['board','irq','timer','uart','gpio','watchdog','i2c','sensor-bootstrap','isp']+(['usb'] if args.usb else [])+(['ptz'] if args.ptz else []):
        obj=output/'arch/arm/mach-ak3918ev200'/(unit+'.o')
        if not obj.is_file():raise SystemExit('Missing compiled platform object '+unit)
        linkage.append({'unit':unit,'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest()})
    names=subprocess.check_output(['llvm-nm',str(output/'vmlinux')],text=True)
    for symbol in ['__mach_desc_AK3918EV200_DT','ak39e2_boot_marker','ak_gpio_probe','ak_wdt_probe','ak_i2c_probe','ak_sensor_bootstrap_probe','ak_isp_probe']+(['ak_usb_probe','musb_interrupt','usb_add_hcd'] if args.usb else [])+(['ak_ptz_probe'] if args.ptz else []):
        if not any(line.split()[-1]==symbol for line in names.splitlines() if line.split()):
            raise SystemExit('Missing linked platform symbol '+symbol)
    zimage=output/'arch/arm/boot/zImage';copy=session/'artifacts/zImage'
    suffix='-attempt'+args.attempt if copy.exists() else ''
    copy=session/'artifacts'/('zImage'+suffix)
    with copy.open('xb') as f:f.write(zimage.read_bytes())
    combined=session/'artifacts'/('zImage-with-candidate-dtb'+suffix)
    with combined.open('xb') as f:f.write(zimage.read_bytes()+dtb.read_bytes())
    finalcfg=session/'artifacts'/('kernel'+suffix+'.config')
    with finalcfg.open('xb') as f:f.write(cfg.read_bytes())
    report={'utc':now(),'kernel_commit':subprocess.check_output(['git','-C',str(kernel),'rev-parse','HEAD'],text=True).strip(),
            'platform_linkage_verified':True,'platform_objects':linkage,
            'ram_layout':'stock81800000..84000000/fixed81808000 output' if args.stock_ram else 'experimental64MiB/full-DRAM layout',
            'qualification_mode':args.qualify,
            'i2c_qualification_mode':args.i2c,
            'sensor_bootstrap_mode':args.sensor,
            'isp_qualification_mode':args.isp,
            'usb_qualification_mode':args.usb,
            'ptz_qualification_mode':args.ptz,
            'radio_ram_loader_mode':args.radio_loader,
            'kernel_release':subprocess.check_output(base+['-s','kernelrelease'],text=True).strip(),
            'artifacts':[{'path':str(p),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [copy,combined,dtb,cpio,init,finalcfg]],
            'status':'cross-built prototype, NOT hardware-booted or qualified',
            'known_missing':'Production networking/storage/V4L2 queues/encoder, full CCF/reset/pinctrl and GPIO IRQ support, radio firmware/host ABI and physical PTZ position/limits remain incomplete. Actual per-image hardware qualifications are retained separately; a compiler result proves none of them.'}
    with (session/'analysis'/('mainline-build'+suffix+'.json')).open('x') as f:json.dump(report,f,indent=2)
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()

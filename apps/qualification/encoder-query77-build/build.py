#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline, fresh Linux 6.18.55/Clang23.1.3 encoder-query qualification build.

No target operations, target execution, external modules, tests or installation.
Each phase writes new evidence; an existing phase receipt is never overwritten.
"""
import argparse
import datetime
import difflib
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[3]
APP = Path(__file__).resolve().parent
BASE = ROOT / 'work/lightbulb-camera/foundation-20261011T041148Z'
OLD = BASE / 'artifacts/native-usb76-source01'
META = BASE / 'analysis/native-usb76-package01'
DOC = ROOT / 'docs/qualification-plans/encoder-query77-build-20261022'
QUERY = ROOT / 'docs/qualification-plans/encoder-identity-20261022'
LOCALVERSION = '-qc07f-encoder-query77'
TOOLS = ('make', 'clang', 'clang++', 'ld.lld', 'llvm-ar', 'llvm-nm',
         'llvm-objcopy', 'llvm-objdump', 'llvm-readelf', 'llvm-strip',
         'llvm-size', 'llvm-strings', 'llvm-ranlib', 'llvm-as', 'llc',
         'sh', 'bash', 'bison', 'flex', 'bc', 'gzip', 'bzip2', 'xz',
         'lz4', 'zstd', 'perl', 'python3', 'awk', 'sed', 'grep', 'cat',
         'find', 'xargs', 'sort', 'head', 'tail', 'cut', 'tr', 'wc',
         'expr', 'basename', 'dirname', 'readlink', 'realpath', 'ln',
         'cp', 'mv', 'rm', 'mkdir', 'mktemp', 'touch', 'date', 'printf', 'uname',
         'm4', 'env', 'tee', 'file', 'stat', 'readelf', 'nm', 'objcopy',
         'od', 'dtc', 'mkimage', 'ldd')


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def pin(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return {'bytes': Path(path).stat().st_size, 'sha256': h.hexdigest()}


def write(path, value):
    with Path(path).open('x') as f:
        json.dump(value, f, indent=2, sort_keys=True)
        f.write('\n')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def inventory(root, omit_git=False, confined=True):
    root = Path(root)
    rows = {}
    for parent, dirs, files in os.walk(root, followlinks=False):
        if omit_git and Path(parent) == root and '.git' in dirs:
            dirs.remove('.git')
        for name in sorted(dirs + files):
            p = Path(parent) / name
            rel = str(p.relative_to(root))
            mode = p.lstat().st_mode
            if stat.S_ISLNK(mode):
                target = os.readlink(p)
                if confined:
                    require(not Path(target).is_absolute(), 'absolute symlink: ' + str(p))
                    require(p.resolve().is_relative_to(root.resolve()), 'escaping symlink: ' + str(p))
                rows[rel] = {'link': target}
            elif stat.S_ISREG(mode):
                rows[rel] = {**pin(p), 'mode': oct(stat.S_IMODE(mode))}
            else:
                require(stat.S_ISDIR(mode), 'special source file: ' + str(p))
    return rows


def verify(root, rows):
    for name, expected in rows.items():
        p = Path(root) / name
        if 'link' in expected:
            require(p.is_symlink() and os.readlink(p) == expected['link'], 'link drift: ' + str(p))
        else:
            require(not p.is_symlink() and pin(p) == {k: expected[k] for k in ('bytes', 'sha256')},
                    'input drift: ' + str(p))


def config(text):
    result = {}
    for line in text.splitlines():
        m = re.fullmatch(r'(CONFIG_\w+)=(.*)', line)
        n = re.fullmatch(r'# (CONFIG_\w+) is not set', line)
        if m:
            result[m[1]] = m[2]
        elif n:
            result[n[1]] = 'n'
    return result


def cpio_rows(data):
    """Inspect newc names/types/links in memory only; never extract or execute."""
    rows = []
    seen = set()
    pos = 0
    while True:
        require(data[pos:pos + 6] == b'070701', 'not newc')
        header = data[pos + 6:pos + 110]
        require(len(header) == 104, 'short newc')
        words = [int(header[i:i + 8], 16) for i in range(0, 104, 8)]
        end_name = pos + 110 + words[11]
        raw_name = data[pos + 110:end_name]
        require(raw_name.endswith(b'\0') and b'\0' not in raw_name[:-1], 'newc name')
        name = raw_name[:-1].decode('ascii').removeprefix('./')
        require(name and not name.startswith('/') and '..' not in Path(name).parts
                and name not in seen, 'unsafe/duplicate newc path')
        seen.add(name)
        start = (end_name + 3) & ~3
        end = start + words[6]
        require(end <= len(data), 'newc extent')
        payload = data[start:end]
        row = {'path': name, 'mode': oct(words[1]), 'uid': words[2], 'gid': words[3],
               'bytes': len(payload), 'sha256': hashlib.sha256(payload).hexdigest()}
        if stat.S_ISLNK(words[1]):
            row['link'] = payload.decode('ascii')
        rows.append(row)
        pos = (end + 3) & ~3
        if name == 'TRAILER!!!':
            require(not any(data[pos:]), 'nonzero newc trailing bytes')
            return rows


def elf(path):
    """Read real ELF32 ARM sections and symbols; no execution or external library."""
    data = Path(path).read_bytes()
    require(data[:7] == b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H', data, 18)[0] == 40,
            'expected ELF32 little-endian ARM')
    off = struct.unpack_from('<I', data, 32)[0]
    size, count = struct.unpack_from('<HH', data, 46)
    require(size == 40 and off + count * size <= len(data), 'ELF section extent')
    sections = [struct.unpack_from('<10I', data, off + i * size) for i in range(count)]
    symbols = {}
    for sec in sections:
        if sec[1] != 2:
            continue
        strings = sections[sec[6]]
        text = data[strings[4]:strings[4] + strings[5]]
        require(sec[9] == 16, 'ELF symbol size')
        for i in range(0, sec[5], 16):
            name, value, length, info, other, index = struct.unpack_from('<IIIBBH', data, sec[4] + i)
            if name:
                symbols[text[name:text.index(b'\0', name)].decode()] = [value, length, index]
    return symbols


def bzip_crc(data):
    # CRC-32/BZIP2: poly04c11db7, init/xorout ffffffff, non-reflected.
    table = []
    for i in range(256):
        n = i << 24
        for _ in range(8):
            n = ((n << 1) ^ (0x04c11db7 if n & 0x80000000 else 0)) & 0xffffffff
        table.append(n)
    n = 0xffffffff
    for b in data:
        n = ((n << 8) & 0xffffffff) ^ table[((n >> 24) ^ b) & 255]
    return n ^ 0xffffffff


class Build:
    def __init__(self, run):
        self.art = BASE / ('artifacts/encoder-query77-source' + run)
        self.out = BASE / ('analysis/encoder-query77-build' + run)
        self.images = BASE / ('artifacts/encoder-query77-images' + run)
        self.src = self.art / 'linux'
        self.build = self.out / 'build'
        self.records = []
        self.env = {'PATH': '/usr/bin:/bin', 'HOME': str(self.out / 'home'),
                    'TMPDIR': str(self.out / 'tmp'), 'LANG': 'C', 'LC_ALL': 'C',
                    'PYTHONDONTWRITEBYTECODE': '1', 'SOURCE_DATE_EPOCH': '1792627200',
                    'KBUILD_BUILD_USER': 'qualification', 'KBUILD_BUILD_HOST': 'offline',
                    'KBUILD_BUILD_TIMESTAMP': '2026-10-22 00:00:00 UTC',
                    'KBUILD_BUILD_VERSION': '1', 'LOCALVERSION': '',
                    'LD_DEBUG': 'files,libs',
                    'LD_DEBUG_OUTPUT': str(self.out / 'loader/trace')}
        # Explicit empty LOCALVERSION and disabled CONFIG_LOCALVERSION_AUTO skip
        # every SCM command in the audited scripts/setlocalversion. No .git copied.
        self.make = ['/usr/bin/make', '-C', str(self.src), 'O=' + str(self.build),
                     'ARCH=arm', 'LLVM=1', 'LLVM_IAS=1', 'HOSTCC=clang',
                     'HOSTCXX=clang++', 'LOCALVERSION=', 'V=1']

    def command(self, label, argv, timeout=2400):
        log = self.out / 'logs' / (label + '.log')
        record = {'argv': list(map(str, argv)), 'cwd': str(ROOT),
                  'environment': self.env, 'started': now(), 'log': str(log.relative_to(ROOT)),
                  'builder_pin': pin(APP / 'build.py')}
        self.records.append(record)
        print('START', label, flush=True)
        try:
            with log.open('xb') as f:
                p = subprocess.run(record['argv'], cwd=ROOT, env=self.env,
                                   stdout=f, stderr=subprocess.STDOUT, timeout=timeout)
            record['exit'] = p.returncode
        finally:
            record['ended'] = now()
            if log.is_file():
                record['log_pin'] = pin(log)
            with (self.out / 'commands.jsonl').open('a') as f:
                f.write(json.dumps(record) + '\n')
        require(p.returncode == 0, 'command failed: ' + label)
        print('END', label, flush=True)
        return log.read_text(errors='replace')

    def prepare(self):
        require(not any(p.exists() or p.is_symlink() for p in (self.art, self.out, self.images)),
                'fresh paths required')
        for p in (self.art, self.out, self.images):
            p.mkdir()
        for n in ('logs', 'loader', 'home', 'tmp', 'inputs'):
            (self.out / n).mkdir()
        self.build.mkdir()
        source = inventory(OLD / 'linux', omit_git=True)
        write(self.out / 'inputs/retained76-source.json', source)
        # Audit and retain SCM metadata only as data, never execute Git or copy it
        # into the source baseline. The full source byte manifest is authoritative.
        scm = inventory(OLD / 'linux/.git')
        write(self.out / 'inputs/retained76-scm.json', scm)
        require((OLD / 'linux/.git/HEAD').read_text().strip() ==
                '725bd2f3c81d54edccb66fead01d4c0c222e2231', 'old source HEAD changed')
        require(not list((OLD / 'linux').glob('localversion*')) and
                not (OLD / 'linux/.scmversion').exists(), 'unreviewed source version file')
        package = json.loads((META / 'package76-manifest.json').read_text())
        custody = {}
        for rel, sha in package['output_sha256'].items():
            p = ROOT / rel
            require(pin(p)['sha256'] == sha, 'old package output drift: ' + rel)
            custody[rel] = pin(p)
        for p in (META / 'package76-manifest.json', *sorted((META / 'recipe-snapshot').glob('*'))):
            if p.is_file():
                custody[str(p.relative_to(ROOT))] = pin(p)
        seal = json.loads((QUERY / 'SOURCE_SEAL01.json').read_text())
        require(seal['source_tree_sha256'] == 'ec3b9daa257ce6bd72648b4ba7c907c405be588ef1e8bb35c787b1932e9e9520', 'query seal')
        for rel, value in (seal['source_files'] | seal['evidence_pins']).items():
            require(pin(ROOT / rel) == value, 'frozen query drift: ' + rel)
            custody[rel] = value
        for p in sorted((ROOT / 'docs/qualification-plans/encoder-identity-clang2313-20261022').rglob('*')):
            if p.is_file():
                custody[str(p.relative_to(ROOT))] = pin(p)
        # Preserve all old final build metadata/objects, not just selected outputs.
        write(self.out / 'inputs/retained76-build.json', inventory(META / 'build', confined=False))
        write(self.out / 'inputs/custody.json', custody)
        require(source['arch/arm/mach-ak3918ev200/qc07f_hwenc.c']['sha256'] ==
                'e97744838acd469772db41fb1fe0907c95fb6e923577cdbffc84ead0141a026f', 'original provider')
        shutil.copytree(OLD / 'linux', self.src, symlinks=True,
                        ignore=lambda p, names: ['.git'] if Path(p) == OLD / 'linux' else [])
        verify(self.src, source)
        board = self.src / 'arch/arm/mach-ak3918ev200'
        patch = QUERY / 'runs/build01/applied-source.patch'
        require(pin(patch)['sha256'] == '9d096489e3ba8cc44556b81feb8192b6c5f08e698c18b2718cdfedbab5aea232', 'patch drift')
        shutil.copyfile(patch, self.out / 'inputs/applied-source.patch')
        c = (board / 'qc07f_hwenc.c').read_text()
        h = (board / 'qc07f_hwenc.h').read_text()
        replacements = [('static long encoder_ioctl(', '#include "encoder-open-query.inc"\nstatic long encoder_ioctl('),
                        (' int result;\n if(command==QC_HWENC_LIFETIME_QUERY){',
                         ' int result;\n if(command==QC_HWENC_OPEN_QUERY)return encoder_open_query(file,argument);\n if(command==QC_HWENC_LIFETIME_QUERY){')]
        for before, after in replacements:
            require(c.count(before) == 1, 'overlay C context')
            c = c.replace(before, after)
        require(h.count('#endif\n') == 1, 'overlay H context')
        h = h.replace('#endif\n', '#include "encoder-open-query.h"\n#endif\n')
        (board / 'qc07f_hwenc.c').write_text(c)
        (board / 'qc07f_hwenc.h').write_text(h)
        for name in ('encoder-open-query.h', 'encoder-open-query.inc'):
            shutil.copyfile(ROOT / 'apps/qualification/kernel76-encoder-identity' / name, board / name)
        for name in ('qc07f_hwenc.c', 'qc07f_hwenc.h', 'encoder-open-query.h', 'encoder-open-query.inc'):
            require((board / name).read_bytes() == (QUERY / 'runs/build01/candidate' / name).read_bytes(), 'candidate differs')
        fresh = inventory(self.src)
        delta = {k: {'old': source.get(k), 'new': fresh.get(k)} for k in source.keys() | fresh.keys()
                 if source.get(k) != fresh.get(k)}
        require(set(delta) == {'arch/arm/mach-ak3918ev200/' + n for n in
                              ('qc07f_hwenc.c', 'qc07f_hwenc.h', 'encoder-open-query.h', 'encoder-open-query.inc')}, 'extra source delta')
        write(self.out / 'inputs/candidate-source.json', fresh)
        write(self.out / 'inputs/source-delta.json', delta)
        (self.art / 'inputs').mkdir()
        for name in ('attempt45.dtb', 'initramfs76.cpio'):
            shutil.copyfile(OLD / 'inputs' / name, self.art / 'inputs' / name)
        init = (OLD / 'inputs/initramfs76.cpio').read_bytes()
        require(gzip.decompress((META / 'build/usr/initramfs_inc_data').read_bytes()) == init,
                'old embedded initramfs not equal selected cpio')
        write(self.out / 'inputs/initramfs-inventory.json', cpio_rows(init))
        oldcfg = (META / 'build/.config').read_text()
        require(config(oldcfg)['CONFIG_INITRAMFS_SOURCE'] == json.dumps(str(OLD / 'inputs/initramfs76.cpio')),
                'actual initramfs selection')
        require(config(oldcfg)['CONFIG_LOCALVERSION_AUTO'] == 'n', 'automatic SCM identity enabled')
        (self.out / 'inputs/retained.config').write_text(oldcfg)
        cfg = oldcfg.replace('CONFIG_LOCALVERSION="-qc07f-prototype"', 'CONFIG_LOCALVERSION="' + LOCALVERSION + '"')
        require(cfg != oldcfg, 'localversion not changed')
        (self.build / '.config').write_text(cfg)
        tools = {}
        for name in TOOLS:
            tool = shutil.which(name, path='/usr/bin:/bin')
            if tool:
                p = Path(tool)
                tools[tool] = {'resolved': str(p.resolve()), **pin(p)}
        write(self.out / 'inputs/host-tools.json', tools)
        # Full installed resource/system-header inventories cover host generators
        # as well as -nostdinc target dependencies; nothing is downloaded.
        write(self.out / 'inputs/clang-resources.json', inventory('/usr/lib/clang/23'))
        # System headers may legitimately link outside /usr/include. Inventory
        # their resolved bytes without imposing source-tree symlink confinement.
        headers = {}
        for root in (Path('/usr/include'), Path('/usr/lib/gcc')):
            if root.exists():
                for p in sorted(root.rglob('*')):
                    if p.is_file():
                        headers[str(p)] = {'resolved': str(p.resolve()), **pin(p)}
        write(self.out / 'inputs/host-headers-and-gcc.json', headers)
        for name in ('clang', 'ld.lld', 'llvm-ar', 'llvm-objcopy', 'make', 'bison', 'flex', 'dtc'):
            text = self.command(name + '-version', ['/usr/bin/' + name, '--version'], 60)
            if name == 'clang':
                require('clang version 23.1.3' in text, 'new compiler not selected23.1.3')
            if name == 'ld.lld':
                require('LLD 23.1.3' in text, 'linker not23.1.3')
        self.command('clang-resource-dir', ['/usr/bin/clang', '-print-resource-dir'], 60)
        write(self.out / 'PREPARED.json', {'ended': now(), 'source_files': len(source),
              'source_delta': delta, 'environment': self.env, 'commands': self.records,
              'retained_scm_copied': False, 'old_module_paired': False})

    def configure(self):
        require((self.out / 'PREPARED.json').is_file(), 'prepare first')
        self.command('olddefconfig', self.make + ['olddefconfig'])
        before = config((self.out / 'inputs/retained.config').read_text())
        after = config((self.build / '.config').read_text())
        delta = {k: {'old': before.get(k), 'new': after.get(k)} for k in before.keys() | after.keys()
                 if before.get(k) != after.get(k)}
        allowed = {'CONFIG_CC_VERSION_TEXT', 'CONFIG_CLANG_VERSION', 'CONFIG_AS_VERSION',
                   'CONFIG_LLD_VERSION', 'CONFIG_LOCALVERSION'}
        (self.out / 'inputs/config.diff').write_text(''.join(difflib.unified_diff(
            (self.out / 'inputs/retained.config').read_text().splitlines(True),
            (self.build / '.config').read_text().splitlines(True), fromfile='retained76.config', tofile='query77.config')))
        write(self.out / 'CONFIG_REVIEW.json', {'ended': now(), 'delta': delta,
              'allowed_identity_symbols': sorted(allowed), 'admitted': set(delta) <= allowed,
              'config_pin': pin(self.build / '.config'), 'commands': self.records})
        require(set(delta) <= allowed, 'selected hardware/feature config changed; STOP before full build')
        require(after['CONFIG_CLANG_VERSION'] == '230103' and after['CONFIG_AS_VERSION'] == '230103'
                and after['CONFIG_LLD_VERSION'] == '230103' and after['CONFIG_LOCALVERSION'] == json.dumps(LOCALVERSION),
                'incoherent new compiler/config')

    def compile(self):
        review = json.loads((self.out / 'CONFIG_REVIEW.json').read_text())
        require(review['admitted'] and pin(self.build / '.config') == review['config_pin'], 'config not admitted/frozen')
        verify(self.src, json.loads((self.out / 'inputs/candidate-source.json').read_text()))
        # zImage -> vmlinux -> built-in modpost provides vmlinux.symvers. An
        # explicit Module.symvers target is not available in this kernel without
        # the module path. Do not manufacture one or select a modules build.
        self.command('kernel-builtins', self.make + ['-j4', 'zImage', 'vmlinux'])
        require(pin(self.build / '.config') == review['config_pin'], 'build changed selected config')
        require((self.build / 'include/config/kernel.release').read_text().strip() == '6.18.55' + LOCALVERSION,
                'unexpected release/SCM identity')
        require(gzip.decompress((self.build / 'usr/initramfs_inc_data').read_bytes()) ==
                (OLD / 'inputs/initramfs76.cpio').read_bytes(), 'new embedded cpio differs')
        self.command('vmlinux-ELF', ['/usr/bin/llvm-readelf', '-h', '-l', '-S', self.build / 'vmlinux'])
        self.command('compressed-ELF', ['/usr/bin/llvm-readelf', '-h', '-l', '-S', self.build / 'arch/arm/boot/compressed/vmlinux'])
        self.command('compressed-symbols', ['/usr/bin/llvm-nm', '-n', self.build / 'arch/arm/boot/compressed/vmlinux'])
        self.command('retained-DTB', ['/usr/bin/dtc', '-I', 'dtb', '-O', 'dts', self.art / 'inputs/attempt45.dtb'])
        write(self.out / 'BUILT.json', {'ended': now(), 'commands': self.records,
              'outputs': {str(p.relative_to(self.build)): pin(p) for p in [self.build / n for n in
                  ('vmlinux', 'arch/arm/boot/zImage', 'arch/arm/boot/Image', 'vmlinux.symvers', 'System.map',
                   '.config', 'usr/initramfs_inc_data', 'include/generated/compile.h',
                   'include/generated/autoconf.h', 'include/config/kernel.release')]},
              'external_modules_built': False, 'outputs_executed': False})

    def precompile(self):
        # Capture only relevant installed tool binaries and observed loaded
        # images; unrelated privileged service executables are not dependencies.
        rows = {}
        for name in TOOLS:
            tool = shutil.which(name, path='/usr/bin:/bin')
            if tool:
                p = Path(tool)
                rows[tool] = {'resolved': str(p.resolve()), **pin(p)}
        for log in sorted((self.out / 'loader').glob('trace.*')):
            for name in re.findall(r'(?:calling init:|calling fini:|file=|transferring control:)\s*(/[^\s;\[\n]+)',
                                   log.read_text(errors='replace')):
                p = Path(name)
                if name not in rows and p.is_file():
                    rows[name] = {'resolved': str(p.resolve()), **pin(p)}
        write(self.out / 'inputs/precompile-tools-and-ELF.json', rows)
        dest = self.out / 'inputs/recipe-at-compile02'
        dest.mkdir()
        for p in sorted(APP.iterdir()):
            if p.is_file():
                shutil.copyfile(p, dest / p.name)
        write(self.out / 'PRECOMPILE.json', {'ended': now(), 'inputs': len(rows),
              'builder_pin': pin(APP / 'build.py'), 'command': [sys.executable, *sys.argv]})

    def closure(self):
        loaded = {}
        # LD_DEBUG records all actually loaded ELF shared libraries, including
        # fresh host generators. Keep unfiltered original build diagnostics.
        for log in [*sorted((self.out / 'logs').glob('*.log')),
                    *sorted((self.out / 'loader').glob('trace.*'))]:
            text = log.read_text(errors='replace')
            paths = re.findall(r'(?:calling init:|calling fini:|file=|transferring control:)\s*(/[^\s;\[\n]+)', text)
            for name in paths:
                p = Path(name)
                if str(p) not in loaded and p.is_file():
                    loaded[str(p)] = {'resolved': str(p.resolve()), **pin(p)}
        for p in (Path('/lib64/ld-linux-x86-64.so.2'),):
            loaded[str(p)] = {'resolved': str(p.resolve()), **pin(p)}
        write(self.out / 'loaded-host-ELF.json', loaded)
        # .cmd includes exact expanded compiler/assembler/linker recipes and
        # actual compiler dependency paths for Kbuild objects and host generators.
        cmds = {str(p.relative_to(self.build)): pin(p) for p in sorted(self.build.rglob('*.cmd'))}
        write(self.out / 'kbuild-command-files.json', cmds)
        consumed = {}
        for path in sorted(self.build.rglob('*.cmd')):
            text = path.read_text(errors='replace').replace('\\\n', ' ')
            for line in text.splitlines():
                if not re.match(r'(?:source_|deps_)\S*\s*:=', line):
                    continue
                deps = re.sub(r'\$\(wildcard ([^)]+)\)', r'\1', line.split(':=', 1)[1])
                for name in deps.split():
                    if '$' in name:
                        continue
                    p = Path(name) if Path(name).is_absolute() else self.build / name
                    if str(p) not in consumed and p.is_file():
                        consumed[str(p)] = {'resolved': str(p.resolve()), **pin(p)}
        write(self.out / 'consumed-kbuild-source-headers.json', consumed)

    def custody(self):
        verify(OLD / 'linux', json.loads((self.out / 'inputs/retained76-source.json').read_text()))
        verify(OLD / 'linux/.git', json.loads((self.out / 'inputs/retained76-scm.json').read_text()))
        verify(META / 'build', json.loads((self.out / 'inputs/retained76-build.json').read_text()))
        verify(ROOT, json.loads((self.out / 'inputs/custody.json').read_text()))
        verify(self.src, json.loads((self.out / 'inputs/candidate-source.json').read_text()))
        files = ['host-tools.json', 'host-headers-and-gcc.json']
        if (self.out / 'inputs/precompile-tools-and-ELF.json').exists():
            files.append('precompile-tools-and-ELF.json')
        for filename in files:
            for name, value in json.loads((self.out / 'inputs' / filename).read_text()).items():
                require(str(Path(name).resolve()) == value['resolved'] and
                        pin(name) == {k: value[k] for k in ('bytes', 'sha256')}, 'tool/header drift: ' + name)
        verify('/usr/lib/clang/23', json.loads((self.out / 'inputs/clang-resources.json').read_text()))
        write(self.out / 'CUSTODY.json', {'ended': now(), 'all_captured_inputs_unchanged': True,
              'fresh_source_only_exact_overlay': True, 'target_or_private_IO': False,
              'Git_or_index_operation': False, 'tests_or_target_execution': False})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=('prepare', 'configure', 'precompile', 'compile', 'closure', 'custody'))
    parser.add_argument('--run', default='01')
    args = parser.parse_args()
    require(re.fullmatch(r'[0-9]{2}', args.run), 'two-digit fresh run suffix')
    b = Build(args.run)
    try:
        getattr(b, args.phase)()
    except BaseException as exc:
        if b.out.exists():
            receipt = b.out / (args.phase.upper() + '_FAILURE.json')
            if not receipt.exists():
                write(receipt, {'ended': now(), 'error': type(exc).__name__ + ': ' + str(exc),
                      'commands': b.records, 'outputs_preserved': True})
        raise


if __name__ == '__main__':
    main()

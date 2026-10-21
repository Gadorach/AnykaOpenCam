#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Separate unadopted existing-guard correction; never imports current campaign files."""
from pathlib import Path
import difflib,json,re

def original_from_receipt(path):
    return json.loads(Path(path).read_text())['text_public_scalars'].split('\r\n')[0]

def candidate(original):
    # Keep FD/fork/exec/parent/PID/counter/wait/nonzero recovery unchanged.
    begin='for n in '+' '.join(map(str,range(1,46)))+'; do '
    end='done; wait $auth04_pid;'
    start=original.index(begin);stop=original.index(end,start)
    old=original[start:stop]
    match=re.fullmatch(re.escape(begin)+r'case "\$\(ls -l /proc/\$auth04_pid/exe\)" in \*" -> ([^"]+)"\) sleep 4; echo \. >&3;; \*\) break;; esac; ',old)
    if not match:raise ValueError('observed legacy guard grammar changed')
    exe=match[1]
    # Source-owned birth/PPID captured before first sleep; does NOT demand immediate
    # exec image at this point. POSIX set/parameter expansion; no awk/stat/ps.
    bind=('guard_exec_seen=0; guard_fault=0; guard_stat=$(cat /proc/$auth04_pid/stat); '
      'guard_tail=${guard_stat##*) }; set -- $guard_tail; guard_ppid=${2}; guard_birth=${20}; '
      'case "$guard_ppid:$guard_birth" in *[!0-9:]*|:|:*|*:) guard_fault=1;; esac; ')
    loop=(begin+'sleep 4; guard_stat=$(cat /proc/$auth04_pid/stat); guard_tail=${guard_stat##*) }; set -- $guard_tail; '
      'case "$guard_stat" in "") echo GUARD_OWN_PID_GONE:$n; break;; esac; '
      'if test "$guard_fault" != 0 || test "${2}" != "$guard_ppid" || test "${20}" != "$guard_birth"; '
      'then guard_fault=1; echo GUARD_SOURCE_REFUSED:$n; break; fi; '
      f'case "$(ls -l /proc/$auth04_pid/exe)" in *" -> {exe}") '
      'if test "$guard_exec_seen" = 0; then echo GUARD_FIRST_EXEC_MATCH:$n; fi; '
      'guard_exec_seen=1; echo . >&3; echo GUARD_PING:$n;; '
      '*) guard_fault=1; echo GUARD_EXE_REFUSED:$n; break;; esac; ')
    result=original[:start]+bind+loop+original[stop:]
    condition='if test $auth04_rc = 0; then printf V >&3; exec 3>&-;'
    if result.count(condition)!=1:raise ValueError('observed close boundary changed')
    result=result.replace(condition,'echo GUARD_WAIT_OBSERVED:$auth04_rc; if test $auth04_rc = 0 && test "$guard_exec_seen" = 1 && test "$guard_fault" = 0; then printf V >&3; exec 3>&-;')
    return result

def diff(original,new):
    return ''.join(difflib.unified_diff([original+'\n'],[new+'\n'],fromfile='observed-native04/guard',tofile='unadopted-candidate/guard'))

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Source-only guard contract tests; no campaign import/target/status/private IO."""
from pathlib import Path
import unittest
from candidate import candidate,original_from_receipt
ROOT=Path(__file__).resolve().parents[4]
INPUTS=ROOT/'evidence/delegated/20261012-native-ui/guard-review/inputs'
class Source(unittest.TestCase):
    def setUp(self):self.old=original_from_receipt(INPUTS/'native04-observed.json');self.new=candidate(self.old)
    def test_same_initial_FD_fork_exec_and_PID_marker(self):
        boundary='for n in '
        before=self.old[:self.old.index(boundary)]
        self.assertTrue(self.new.startswith(before));self.assertIn('(exec 3>&-; exec /run/auth-launch-signal04.elf',self.new)
        self.assertEqual(self.new.count('/dev/watchdog0'),1)
    def test_same45_counter4sleep_no_timer_or_cadence_relax(self):
        count='for n in '+' '.join(map(str,range(1,46)))+'; do '
        self.assertEqual(self.new.count(count),1);self.assertEqual(self.new.count('sleep 4;'),1);self.assertIn(count+'sleep 4;',self.new)
        self.assertIn('sleep 6;',self.new) # original uncertain/nonzero recovery unchanged
    def test_check_after_existing_sleep_and_birth_before_sleep(self):
        self.assertLess(self.new.index('guard_birth=${20}'),self.new.index('for n in '))
        self.assertLess(self.new.index('do sleep 4;'),self.new.index('case "$(ls -l /proc/$auth04_pid/exe)"'))
    def test_onlywait0_and_execadmission_no_fault_magicclose(self):
        self.assertIn('wait $auth04_pid; auth04_rc=$?',self.new)
        self.assertIn('if test $auth04_rc = 0 && test "$guard_exec_seen" = 1 && test "$guard_fault" = 0; then printf V >&3; exec 3>&-',self.new)
        self.assertEqual(self.new.count('printf V'),1)
    def test_no_arbitraryPID_respawn_signal_or_private_print(self):
        for word in ['kill ','ps ','awk ','rmdir ','rm -rf','/cmdline','/environ','env ','restart','arm_abort']:
            self.assertNotIn(word,self.new)
        self.assertEqual(self.new.count('& auth04_pid=$!'),1)
        self.assertIn('GUARD_WAIT_OBSERVED:$auth04_rc',self.new)
    def test_original_is_known_immediatecheck_hazard(self):
        self.assertIn('do case "$(ls -l /proc/$auth04_pid/exe)"',self.old)
        self.assertIn('*) break;; esac; done; wait $auth04_pid;',self.old)
        self.assertNotIn('GUARD_FIRST_EXEC_MATCH',self.old)
    def test_unknown_original_grammar_refused(self):
        with self.assertRaises(ValueError):candidate(self.old.replace('sleep 4;','sleep 5;'))
if __name__=='__main__':unittest.main(verbosity=2)

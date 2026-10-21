"""Public host fixtures only: no camera, UART, network or firmware execution."""

import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


RUNNER = Path(__file__).resolve().parents[1] / "foundation_job_runner.py"
SPEC = importlib.util.spec_from_file_location("foundation_job_runner", RUNNER)
runner = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runner)


class JobRunnerTest(unittest.TestCase):
    def setUp(self):
        parent = os.environ.get("TMPDIR") or ("/tmp/opencode" if Path("/tmp/opencode").is_dir() else None)
        self.temp = tempfile.TemporaryDirectory(prefix="foundation-job-test-", dir=parent)
        self.root = Path(self.temp.name)
        self.source = self.root / "public-source.bin"
        self.source.write_bytes(b"dummy-public-fixture\n" * 3)
        self.pin = hashlib.sha256(self.source.read_bytes()).hexdigest()
        self.output = self.root / "evidence"
        # A real, finite file transform with count/source integrity evidence.
        program = (
            "import hashlib,json,pathlib,sys; "
            "data=pathlib.Path(sys.argv[1]).read_bytes(); "
            "pathlib.Path('copy.bin').write_bytes(data); "
            "pathlib.Path('result.json').write_text(json.dumps({"
            "'completed':True,'count':3,'source_sha256':hashlib.sha256(data).hexdigest(),"
            "'decoder_errors':[],'matches':True})); "
            "print('TRANSFORM COMPLETE')"
        )
        self.config = {
            "schema": 1, "task_id": "BASE-07", "evidence_class": "fixture",
            "identity": {"source_revision": self.pin, "generation": "dummy-public-01", "target": None},
            "public_data_only": True, "budget_seconds": 5,
            "max_artifact_bytes": 1024 * 1024, "max_artifact_files": 10,
            "tool_versions": {"python": sys.version.split()[0]},
            "inputs": [{"path": str(self.source), "sha256": self.pin, "sensitive": False}],
            "artifacts": ["copy.bin", "result.json"],
            "commands": [{
                "id": "transform", "argv": [sys.executable, "-c", program, str(self.source)],
                "timeout_seconds": 2, "max_log_bytes": 4096,
                "sensitive_argv": False, "sensitive_logs": False, "decoder": False,
                "proofs": [
                    {"kind": "json", "path": "result.json", "role": "completion", "field": ["completed"], "equals": True},
                    {"kind": "json", "path": "result.json", "role": "observation", "field": ["count"], "minimum": 3, "maximum": 3},
                    {"kind": "json", "path": "result.json", "role": "observation", "field": ["decoder_errors"], "length": 0},
                    {"kind": "json", "path": "result.json", "role": "source", "field": ["source_sha256"], "equals": self.pin},
                    {"kind": "file", "path": "copy.bin", "role": "source", "sha256": self.pin},
                ],
            }],
        }

    def tearDown(self):
        self.temp.cleanup()

    def run_job(self, config=None):
        result = runner.run(config or self.config, self.output)
        self.assertEqual(result, json.loads((self.output / "summary.json").read_text()))
        return result

    def test_valid_fixture_hashes_timestamps_no_acceptance(self):
        result = self.run_job()
        self.assertEqual(result["status"], "fixture-passed")
        self.assertFalse(result["accepted"])
        self.assertTrue(result["started_utc"])
        self.assertTrue(result["ended_utc"])
        self.assertEqual(result["inputs"][0]["sha256"], self.pin)
        self.assertEqual(result["inputs"][0]["final_sha256"], self.pin)
        self.assertTrue(result["commands"][0]["observation_complete"])
        self.assertEqual(len(result["artifacts"]), 2)
        self.assertTrue(all("sha256" in a for a in result["artifacts"]))
        self.assertNotIn("environment", result)

    def test_nonzero_exit_retains_artifacts_and_logs(self):
        self.config["commands"][0]["argv"][2] += "; print('public failure',file=sys.stderr); sys.exit(7)"
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertEqual(result["commands"][0]["returncode"], 7)
        self.assertIn("public failure", (self.output / "commands/transform/stderr.log").read_text())
        self.assertEqual((self.output / "copy.bin").read_bytes(), self.source.read_bytes())

    def test_timeout_retains_partial_artifact_and_never_passes(self):
        self.config["commands"][0]["timeout_seconds"] = 0.2
        self.config["commands"][0]["argv"][2] += (
            "; print('partial observation',flush=True)\n"
            "while True: hashlib.sha256(data).digest()"
        )
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertFalse(result["commands"][0]["observation_complete"])
        # Even all satisfied predicates cannot rescue a timed-out command.
        self.assertTrue(all(p["passed"] for p in result["commands"][0]["proofs"]))
        self.assertIn("timeout", " ".join(result["commands"][0]["errors"]))
        self.assertTrue((self.output / "copy.bin").exists())

    def test_missing_artifact_exit_zero_fails(self):
        self.config["artifacts"].append("required-decoded-frame.bin")
        result = self.run_job()
        self.assertEqual(result["commands"][0]["returncode"], 0)
        self.assertEqual(result["status"], "failed")
        self.assertIn("required artifact missing", " ".join(result["errors"]))

    def test_existing_directory_and_failure_not_overwritten(self):
        self.config["commands"][0]["argv"][2] += "; sys.exit(1)"
        self.run_job()
        old = (self.output / "summary.json").read_bytes()
        with self.assertRaises(FileExistsError):
            runner.run(self.config, self.output)
        self.assertEqual(old, (self.output / "summary.json").read_bytes())

    def test_decoder_error_exit_zero_fails(self):
        cmd = self.config["commands"][0]
        cmd["decoder"] = True
        cmd["argv"][2] += "; print('[h264] error while decoding MB 1 2',file=sys.stderr)"
        result = self.run_job()
        self.assertEqual(result["commands"][0]["returncode"], 0)
        self.assertEqual(result["status"], "failed")
        self.assertIn("decoder errors", " ".join(result["commands"][0]["errors"]))

    def test_partial_counts_exit_zero_fails(self):
        self.config["commands"][0]["argv"][2] = self.config["commands"][0]["argv"][2].replace("'count':3", "'count':2")
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertEqual(result["commands"][0]["returncode"], 0)
        self.assertFalse(result["commands"][0]["proofs"][1]["passed"])

    def test_source_proof_exit_zero_fails(self):
        self.config["commands"][0]["argv"][2] = self.config["commands"][0]["argv"][2].replace("hashlib.sha256(data).hexdigest()", "'bad-source'")
        self.assertEqual(self.run_job()["status"], "failed")

    def test_source_input_pin_failure_does_not_launch(self):
        self.source.write_bytes(b"changed source")
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertEqual(result["commands"], [])
        self.assertFalse((self.output / "copy.bin").exists())

    def test_source_modified_during_job_fails(self):
        self.config["commands"][0]["argv"][2] += "; pathlib.Path(sys.argv[1]).write_bytes(b'changed')"
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertIn("source/input changed", " ".join(result["errors"]))

    def test_sensitive_jobs_refused_before_persisting(self):
        for key in ("sensitive_argv", "sensitive_logs"):
            config = copy.deepcopy(self.config)
            config["commands"][0][key] = True
            with self.assertRaises(ValueError):
                runner.run(config, self.output)
            self.assertFalse(self.output.exists())
        config = copy.deepcopy(self.config)
        config["inputs"][0]["sensitive"] = True
        with self.assertRaises(ValueError):
            runner.run(config, self.output)
        self.assertFalse(self.output.exists())

    def test_completion_proof_required_and_missing_marker_fails(self):
        config = copy.deepcopy(self.config)
        config["commands"][0]["proofs"] = [config["commands"][0]["proofs"][1]]
        with self.assertRaises(ValueError):
            runner.run(config, self.output)
        self.config["commands"][0]["proofs"][0] = {
            "kind": "text", "path": "stdout", "role": "completion", "contains": "TARGET ACTUALLY FINISHED"
        }
        self.assertEqual(self.run_job()["status"], "failed")

    def test_log_budget_fails_retains_prefix(self):
        self.config["commands"][0]["max_log_bytes"] = 8
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertEqual((self.output / "commands/transform/stdout.log").stat().st_size, 8)
        self.assertFalse(result["commands"][0]["observation_complete"])

    def test_artifact_budget_fails_retains_file(self):
        self.config["max_artifact_bytes"] = 100
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertTrue((self.output / "copy.bin").exists())

    def test_host_and_explicit_target_class_are_not_acceptance(self):
        self.config["evidence_class"] = "host"
        self.assertEqual(self.run_job()["status"], "host-passed")
        self.config["evidence_class"] = "target"
        with self.assertRaises(ValueError):
            runner.validate(self.config)
        # Validation only: never run a target-labelled synthetic fixture.
        self.config["identity"]["target"] = "EXPLICIT-IDENTITY-SCHEMA-FIXTURE"
        runner.validate(self.config)

    def test_symlink_artifact_fails(self):
        self.config["commands"][0]["argv"][2] += "; pathlib.Path('copy.bin').unlink(); pathlib.Path('copy.bin').symlink_to(sys.argv[1])"
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertTrue((self.output / "copy.bin").is_symlink())

    def test_explicit_version_command_and_batch(self):
        version = copy.deepcopy(self.config["commands"][0])
        version.update(id="python-version", argv=[sys.executable, "--version"],
                       proofs=[{"kind": "text", "path": "stdout", "role": "completion",
                                "contains": "Python " + sys.version.split()[0], "count": 1}])
        self.config["commands"].insert(0, version)
        result = self.run_job()
        self.assertEqual(result["status"], "fixture-passed")
        self.assertEqual(len(result["commands"]), 2)
        self.assertEqual(result["commands"][0]["argv"], [sys.executable, "--version"])

    def test_failed_batch_does_not_run_next_command(self):
        second = copy.deepcopy(self.config["commands"][0])
        second["id"] = "must-not-run"
        self.config["commands"].append(second)
        self.config["commands"][0]["argv"][2] += "; sys.exit(8)"
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertEqual(len(result["commands"]), 1)
        self.assertFalse((self.output / "commands/must-not-run").exists())

    def test_malformed_json_retained_and_fails(self):
        self.config["commands"][0]["argv"][2] += "; pathlib.Path('result.json').write_text('{partial')"
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertEqual(result["commands"][0]["returncode"], 0)
        self.assertEqual((self.output / "result.json").read_text(), "{partial")

    def test_explicit_custom_error_marker_fails(self):
        cmd = self.config["commands"][0]
        cmd["argv"][2] += "; print('PUBLIC CUSTOM DECODE FAILURE',file=sys.stderr)"
        cmd["proofs"].append({"kind": "text", "path": "stderr", "role": "observation",
                              "absent": "PUBLIC CUSTOM DECODE FAILURE"})
        self.assertEqual(self.run_job()["status"], "failed")

    def test_child_does_not_inherit_ambient_environment(self):
        self.config["commands"][0]["argv"][2] += "; import os; assert 'FOUNDATION_AMBIENT_DUMMY' not in os.environ"
        with mock.patch.dict(os.environ, {"FOUNDATION_AMBIENT_DUMMY": "public-dummy"}):
            result = self.run_job()
        self.assertEqual(result["status"], "fixture-passed")
        self.assertNotIn("public-dummy", (self.output / "manifest.json").read_text())

    def test_launch_failure_recorded(self):
        self.config["commands"][0]["argv"][0] = str(self.root / "nonexistent-public-tool")
        result = self.run_job()
        self.assertEqual(result["status"], "failed")
        self.assertIsNone(result["commands"][0]["returncode"])
        self.assertIn("launch/capture", " ".join(result["commands"][0]["errors"]))

    def test_sigterm_finalizes_failure_and_retains_artifacts(self):
        self.config["commands"][0]["argv"][2] += "; import os,signal; os.kill(os.getppid(),signal.SIGTERM)"
        manifest = self.root / "public-manifest.json"
        manifest.write_text(json.dumps(self.config))
        child = subprocess.run(
            [sys.executable, str(RUNNER), "--manifest", str(manifest), "--output", str(self.output)],
            capture_output=True, text=True, timeout=10,
            env={"PATH": os.defpath, "PYTHONDONTWRITEBYTECODE": "1"})
        self.assertEqual(child.returncode, 1, child.stderr)
        self.assertEqual(json.loads(child.stdout)["status"], "failed")
        summary = json.loads((self.output / "summary.json").read_text())
        self.assertEqual(summary["status"], "failed")
        self.assertTrue(summary["ended_utc"])
        self.assertEqual((self.output / "copy.bin").read_bytes(), self.source.read_bytes())

    def test_cli_completion_summary_and_failure_code(self):
        manifest = self.root / "public-manifest.json"
        manifest.write_text(json.dumps(self.config))
        command = [sys.executable, str(RUNNER), "--manifest", str(manifest), "--output", str(self.output)]
        child = subprocess.run(command, capture_output=True, text=True, timeout=10,
                               env={"PATH": os.defpath, "PYTHONDONTWRITEBYTECODE": "1"})
        self.assertEqual(child.returncode, 0, child.stderr)
        message = json.loads(child.stdout)
        self.assertEqual(message["status"], "fixture-passed")
        self.assertEqual(Path(message["summary"]), self.output / "summary.json")
        retry = subprocess.run(command, capture_output=True, text=True, timeout=10,
                               env={"PATH": os.defpath, "PYTHONDONTWRITEBYTECODE": "1"})
        self.assertEqual(retry.returncode, 2)
        self.assertEqual(json.loads(retry.stdout)["status"], "refused")


if __name__ == "__main__":
    unittest.main()

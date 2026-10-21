#!/usr/bin/env python3
"""BASE-07: bounded public-data argv batches; no hardware acceptance policy."""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import selectors
import signal
import stat
import subprocess
import sys
import time
from datetime import datetime, timezone


SCHEMA = 1
DECODER_ERRORS = (
    "error while decoding", "error decoding", "invalid data found",
    "corrupt decoded frame", "concealing", "decode_slice_header error",
    "non-existing pps", "missing picture in access unit", "no frame!",
    "cabac decode", "bytestream overread", "mb_type", "out of range",
    "invalid nal unit", "reference picture missing", "could not find ref",
    "mmco: unref", "left block unavailable", "top block unavailable",
    "decode error", "decoding error", "corrupt input packet", "cbp too large",
)


def utc():
    return datetime.now(timezone.utc).isoformat()


def digest(path, max_bytes=1024 ** 3):
    h = hashlib.sha256()
    total = 0
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            total += len(chunk)
            if total > max_bytes:
                raise ValueError("hashing byte bound exceeded")
            h.update(chunk)
    return h.hexdigest()


def fields(obj, required, optional=()):
    if not isinstance(obj, dict) or set(obj) - set(required) - set(optional):
        raise ValueError("unsupported manifest fields")
    if set(required) - set(obj):
        raise ValueError("missing required manifest fields")


def number(value, maximum):
    if type(value) not in (int, float) or not math.isfinite(value) or not 0 < value <= maximum:
        raise ValueError("invalid positive resource bound")


def text(value):
    if not isinstance(value, str) or not value or "\0" in value:
        raise ValueError("invalid nonempty text")


def relative(value):
    text(value)
    p = Path(value)
    if p.is_absolute() or ".." in p.parts or p == Path("."):
        raise ValueError("artifact paths must be confined relative paths")
    if p.parts[0] in ("commands", "summary.json", "manifest.json"):
        raise ValueError("artifact path conflicts with runner records")


def validate(config):
    fields(config, ("schema", "task_id", "evidence_class", "identity", "public_data_only",
                    "budget_seconds", "max_artifact_bytes", "max_artifact_files",
                    "tool_versions", "inputs", "artifacts", "commands"))
    if type(config["schema"]) is not int or config["schema"] != SCHEMA or config["public_data_only"] is not True:
        raise ValueError("only schema 1 public-data jobs are supported")
    if len(json.dumps(config, allow_nan=False).encode("utf-8")) > 256 * 1024:
        raise ValueError("manifest exceeds 256 KiB")
    text(config["task_id"])
    if config["evidence_class"] not in ("fixture", "host", "target"):
        raise ValueError("explicit fixture, host or target evidence_class required")
    fields(config["identity"], ("source_revision", "generation", "target"))
    text(config["identity"]["source_revision"])
    text(config["identity"]["generation"])
    if config["evidence_class"] == "target":
        text(config["identity"]["target"])
    elif config["identity"]["target"] is not None:
        raise ValueError("non-target jobs must not claim a target identity")
    number(config["budget_seconds"], 86400)
    number(config["max_artifact_bytes"], 1024 ** 3)
    number(config["max_artifact_files"], 10000)
    if type(config["max_artifact_files"]) is not int:
        raise ValueError("artifact count must be an integer")
    if not isinstance(config["tool_versions"], dict) or not config["tool_versions"]:
        raise ValueError("explicit tool versions required (no automatic probing)")
    for name, version in config["tool_versions"].items():
        text(name)
        text(version)
    if not isinstance(config["inputs"], list) or not config["inputs"]:
        raise ValueError("at least one pinned public source/input required")
    for item in config["inputs"]:
        fields(item, ("path", "sha256", "sensitive"))
        text(item["path"])
        if item["sensitive"] is not False:
            raise ValueError("sensitive inputs are unsupported")
        if not isinstance(item["sha256"], str) or len(item["sha256"]) != 64 or any(
                c not in "0123456789abcdef" for c in item["sha256"]):
            raise ValueError("inputs require lowercase SHA256 pins")
    if not isinstance(config["artifacts"], list):
        raise ValueError("artifacts must be a list")
    for path in config["artifacts"]:
        relative(path)
    if len(set(config["artifacts"])) != len(config["artifacts"]):
        raise ValueError("duplicate artifacts")
    if not isinstance(config["commands"], list) or not 1 <= len(config["commands"]) <= 100:
        raise ValueError("one to 100 commands required")
    names = set()
    for cmd in config["commands"]:
        fields(cmd, ("id", "argv", "timeout_seconds", "max_log_bytes", "sensitive_argv",
                     "sensitive_logs", "decoder", "proofs"), ("cwd",))
        name = cmd["id"]
        text(name)
        if any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in name) or name in names:
            raise ValueError("command IDs must be unique simple names")
        names.add(name)
        if cmd["sensitive_argv"] is not False or cmd["sensitive_logs"] is not False:
            raise ValueError("secret-carrying argv/log jobs are unsupported")
        if not isinstance(cmd["argv"], list) or not cmd["argv"]:
            raise ValueError("nonempty argv list required; shell evaluation is unsupported")
        for arg in cmd["argv"]:
            if not isinstance(arg, str) or "\0" in arg:
                raise ValueError("argv must contain strings")
        text(cmd["argv"][0])
        if "cwd" in cmd:
            text(cmd["cwd"])
            if not Path(cmd["cwd"]).is_absolute():
                raise ValueError("explicit cwd must be absolute")
        number(cmd["timeout_seconds"], config["budget_seconds"])
        number(cmd["max_log_bytes"], 64 * 1024 ** 2)
        if type(cmd["max_log_bytes"]) is not int or type(cmd["decoder"]) is not bool:
            raise ValueError("invalid log bound or decoder declaration")
        if not isinstance(cmd["proofs"], list) or not cmd["proofs"]:
            raise ValueError("explicit command proof required")
        roles = set()
        for proof in cmd["proofs"]:
            fields(proof, ("kind", "path", "role"), ("contains", "absent", "count",
                   "equals", "minimum", "maximum", "length", "field", "min_bytes", "sha256"))
            if proof["role"] not in ("completion", "source", "observation"):
                raise ValueError("invalid proof role")
            roles.add(proof["role"])
            if proof["path"] not in ("stdout", "stderr") and proof["path"] not in config["artifacts"]:
                raise ValueError("proof must name a captured log or declared artifact")
            kind = proof["kind"]
            allowed = {"file": {"min_bytes", "sha256"},
                       "text": {"contains", "absent", "count"},
                       "json": {"field", "equals", "minimum", "maximum", "length"}}
            if kind not in allowed or set(proof) - {"kind", "path", "role"} - allowed[kind]:
                raise ValueError("unsupported proof kind/fields")
            checks = set(proof) & allowed[kind] - {"field"}
            if not checks:
                raise ValueError("proof needs a meaningful check")
            if kind == "text":
                for key in ("contains", "absent"):
                    if key in proof:
                        text(proof[key])
                if "count" in proof and ("contains" not in proof or type(proof["count"]) is not int or proof["count"] < 1):
                    raise ValueError("positive literal count requires contains")
            if kind == "json":
                if not isinstance(proof.get("field"), list) or any(type(x) not in (str, int) for x in proof["field"]):
                    raise ValueError("JSON field must be an explicit key/index list")
                if "length" in proof and (type(proof["length"]) is not int or proof["length"] < 0):
                    raise ValueError("invalid expected JSON length")
                for key in ("minimum", "maximum"):
                    if key in proof and (type(proof[key]) not in (int, float) or not math.isfinite(proof[key])):
                        raise ValueError("invalid JSON numeric bound")
            if kind == "file" and "min_bytes" in proof:
                number(proof["min_bytes"], config["max_artifact_bytes"])
            if kind == "file" and "sha256" in proof:
                pin = proof["sha256"]
                if not isinstance(pin, str) or len(pin) != 64 or any(c not in "0123456789abcdef" for c in pin):
                    raise ValueError("artifact SHA256 must be a lowercase pin")
        if "completion" not in roles:
            raise ValueError("each command requires positive completion proof")
        if not any(p["role"] == "completion" and
                   (p["kind"] == "json" and p.get("field") and p.get("equals") or
                    p["kind"] == "text" and "contains" in p) for p in cmd["proofs"]):
            raise ValueError("completion requires a positive JSON value or text marker")
        if config["evidence_class"] == "target" and "source" not in roles:
            raise ValueError("target commands require source-correlation proof")


def regular(path):
    if not stat.S_ISREG(path.lstat().st_mode):
        raise ValueError("not a regular non-symlink file")


def confined(root, relative_path):
    path = root / relative_path
    if any(p.is_symlink() for p in (path, *path.parents)):
        raise ValueError("symlink evidence is unsupported")
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError("evidence escaped output directory")
    regular(path)
    return path


def check_proof(proof, root, logs, limit):
    result = {"predicate": proof, "passed": False}
    try:
        path = logs[proof["path"]] if proof["path"] in logs else confined(root, proof["path"])
        regular(path)
        if path.stat().st_size > (limit if proof["kind"] == "file" else min(limit, 8 * 1024 ** 2)):
            raise ValueError("proof file exceeds evidence bound")
        if proof["kind"] == "file":
            if path.stat().st_size < proof.get("min_bytes", 1):
                raise ValueError("artifact too short")
            if "sha256" in proof and digest(path) != proof["sha256"]:
                raise ValueError("artifact SHA256 mismatch")
        elif proof["kind"] == "text":
            value = path.read_text(encoding="utf-8", errors="strict")
            if "contains" in proof and proof["contains"] not in value:
                raise ValueError("required marker missing")
            if "absent" in proof and proof["absent"] in value:
                raise ValueError("forbidden marker present")
            if "count" in proof and value.count(proof["contains"]) != proof["count"]:
                raise ValueError("marker count mismatch")
        else:
            value = json.loads(path.read_text(encoding="utf-8"))
            for part in proof["field"]:
                value = value[part]
            if "equals" in proof and (type(value) is not type(proof["equals"]) or value != proof["equals"]):
                raise ValueError("JSON value mismatch")
            if "length" in proof and (type(value) not in (list, dict, str) or len(value) != proof["length"]):
                raise ValueError("JSON length mismatch")
            for key, compare in (("minimum", lambda a, b: a >= b), ("maximum", lambda a, b: a <= b)):
                if key in proof and (type(value) not in (int, float) or not math.isfinite(value) or not compare(value, proof[key])):
                    raise ValueError("JSON numeric bound failed")
        result["passed"] = True
    except (OSError, ValueError, TypeError, KeyError, IndexError) as exc:
        result["error"] = str(exc)
    return result


def kill_group(proc):
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass


def execute_command(cmd, root, deadline):
    directory = root / "commands" / cmd["id"]
    directory.mkdir(parents=True)
    logs = {key: directory / (key + ".log") for key in ("stdout", "stderr")}
    argv = [a.replace("{output}", str(root)) for a in cmd["argv"]]
    record = {"id": cmd["id"], "argv": argv, "cwd": cmd.get("cwd", str(root)),
              "started_utc": utc(), "status": "running", "errors": [], "returncode": None,
              "observation_complete": False}
    start = time.monotonic()
    stop = min(deadline, start + cmd["timeout_seconds"])
    proc = None
    captured = 0
    try:
        with logs["stdout"].open("xb") as out, logs["stderr"].open("xb") as err, selectors.DefaultSelector() as selector:
            proc = subprocess.Popen(argv, cwd=record["cwd"], stdin=subprocess.DEVNULL,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    start_new_session=True, env={"PATH": os.defpath, "LANG": "C.UTF-8", "LC_ALL": "C.UTF-8"})
            for pipe, dest in ((proc.stdout, out), (proc.stderr, err)):
                os.set_blocking(pipe.fileno(), False)
                selector.register(pipe, selectors.EVENT_READ, dest)
            while selector.get_map() or proc.poll() is None:
                if time.monotonic() >= stop:
                    record["errors"].append("timeout: observation incomplete")
                    break
                for key, _ in selector.select(min(0.05, max(0, stop - time.monotonic()))):
                    data = os.read(key.fileobj.fileno(), 65536)
                    if not data:
                        selector.unregister(key.fileobj)
                        key.fileobj.close()
                        continue
                    remaining = cmd["max_log_bytes"] - captured
                    key.data.write(data[:remaining])
                    captured += min(len(data), remaining)
                    if len(data) > remaining:
                        record["errors"].append("log limit exceeded: observation incomplete")
                        break
                if record["errors"]:
                    break
            if not record["errors"]:
                # A launcher returning while descendants remain is not job completion.
                try:
                    os.killpg(proc.pid, 0)
                    record["errors"].append("descendants still running: observation incomplete")
                except ProcessLookupError:
                    record["observation_complete"] = True
    except (OSError, ValueError, KeyboardInterrupt) as exc:
        record["errors"].append("command launch/capture interrupted/failed: " + (str(exc) or type(exc).__name__))
    finally:
        if proc is not None:
            if not record["observation_complete"]:
                kill_group(proc)
            proc.wait(timeout=5)
            record["returncode"] = proc.returncode
            for pipe in (proc.stdout, proc.stderr):
                if pipe is not None:
                    pipe.close()
        record["ended_utc"] = utc()
        record["duration_seconds"] = round(time.monotonic() - start, 6)
        record["logs"] = {k: {"path": str(v.relative_to(root)), "bytes": v.stat().st_size,
                              "sha256": digest(v)} for k, v in logs.items() if v.exists()}
    if record["returncode"] != 0:
        record["errors"].append("command did not exit zero")
    if cmd["decoder"]:
        for key, path in logs.items():
            if path.exists():
                value = path.read_bytes().decode("utf-8", errors="replace").lower()
                hits = [marker for marker in DECODER_ERRORS if marker in value]
                if hits:
                    record["errors"].append("decoder errors in " + key + ": " + ", ".join(hits))
    record["status"] = "failed" if record["errors"] else "observed"
    return record, logs


def inventory(root, config):
    items, errors, total = [], [], 0
    directories = 0
    for base, dirs, files in os.walk(root, followlinks=False, onerror=lambda exc: errors.append(str(exc))):
        directories += 1
        if directories > config["max_artifact_files"] + 1:
            errors.append("artifact directory count bound exceeded")
            break
        if Path(base) == root:
            dirs[:] = [d for d in dirs if d != "commands"]
            files = [f for f in files if f not in ("summary.json", "manifest.json")]
        for name in list(dirs):
            if (Path(base) / name).is_symlink():
                errors.append("symlink artifact directory: " + str((Path(base) / name).relative_to(root)))
                dirs.remove(name)
        for name in files:
            path = Path(base) / name
            item = {"path": str(path.relative_to(root))}
            items.append(item)
            if len(items) > config["max_artifact_files"]:
                errors.append("artifact count bound exceeded")
                return items, errors
            try:
                regular(path)
                item["bytes"] = path.stat().st_size
                total += item["bytes"]
                if total > config["max_artifact_bytes"]:
                    raise ValueError("artifact byte budget exceeded; file retained without hashing")
                item["sha256"] = digest(path)
            except (OSError, ValueError) as exc:
                item["error"] = str(exc)
                errors.append(item["path"] + ": " + str(exc))
    return items, errors


def save(root, summary):
    # Atomic checkpoints; command observations/failures only accumulate.
    temporary = root / "summary.json.tmp"
    temporary.write_text(json.dumps(summary, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    temporary.replace(root / "summary.json")


def run(config, output):
    validate(config)  # Reject sensitive jobs before persisting any manifest/argv.
    root = Path(output).absolute()
    root.mkdir(parents=True, exist_ok=False)
    summary = {"schema": SCHEMA, "task_id": config["task_id"], "status": "planned",
               "evidence_class": config["evidence_class"], "identity": config["identity"],
               "accepted": False, "started_utc": utc(), "ended_utc": None,
               "output_directory": str(root), "tool_versions": config["tool_versions"],
               "inputs": [], "artifacts": [], "commands": [], "errors": []}
    (root / "manifest.json").write_text(json.dumps(config, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    summary["manifest_sha256"] = digest(root / "manifest.json")
    save(root, summary)
    deadline = time.monotonic() + config["budget_seconds"]
    try:
        input_bytes = 0
        for item in config["inputs"]:
            path = Path(item["path"]).absolute()
            regular(path)
            size = path.stat().st_size
            input_bytes += size
            if input_bytes > config["max_artifact_bytes"]:
                raise ValueError("total inputs exceed hashing byte bound")
            actual = digest(path, config["max_artifact_bytes"])
            summary["inputs"].append({"path": str(path), "bytes": size, "sha256": actual, "expected_sha256": item["sha256"]})
            if actual != item["sha256"]:
                raise ValueError("input SHA256 mismatch")
        summary["status"] = "running"
        save(root, summary)
        for cmd in config["commands"]:
            if time.monotonic() >= deadline:
                summary["errors"].append("batch deadline reached; remaining commands not run")
                break
            summary["commands"].append({"id": cmd["id"], "status": "running", "started_utc": utc()})
            save(root, summary)
            record, logs = execute_command(cmd, root, deadline)
            record["proofs"] = [check_proof(p, root, logs, config["max_artifact_bytes"]) for p in cmd["proofs"]]
            if not all(p["passed"] for p in record["proofs"]):
                record["errors"].append("required evidence proof failed")
                record["status"] = "failed"
            summary["commands"][-1] = record
            save(root, summary)
            if record["status"] == "failed":
                summary["errors"].append("command failed: " + cmd["id"])
                break  # Explicit sequence, not a dependency/orchestration language.
        for item in summary["inputs"]:
            path = Path(item["path"])
            regular(path)
            item["final_sha256"] = digest(path, config["max_artifact_bytes"])
            if item["final_sha256"] != item["sha256"]:
                summary["errors"].append("source/input changed during job")
    except (OSError, ValueError, KeyboardInterrupt) as exc:
        summary["errors"].append("job interrupted/failed: " + (str(exc) or type(exc).__name__))
    finally:
        summary["artifacts"], errors = inventory(root, config)
        summary["errors"].extend(errors)
        paths = {a["path"] for a in summary["artifacts"] if "sha256" in a}
        for path in config["artifacts"]:
            if path not in paths:
                summary["errors"].append("required artifact missing/incomplete: " + path)
        if len(summary["commands"]) != len(config["commands"]) or any(
                c["status"] != "observed" for c in summary["commands"]):
            summary["errors"].append("not all commands completed")
        summary["status"] = "failed" if summary["errors"] else {
            "fixture": "fixture-passed", "host": "host-passed", "target": "target-passed"
        }[config["evidence_class"]]
        summary["ended_utc"] = utc()
        save(root, summary)
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--output", required=True, help="fresh directory; existing paths refused")
    args = parser.parse_args()
    def interrupted(signum, frame):
        raise KeyboardInterrupt("signal " + str(signum))

    previous = signal.signal(signal.SIGTERM, interrupted)
    try:
        path = Path(args.manifest)
        if path.stat().st_size > 256 * 1024:
            raise ValueError("manifest exceeds 256 KiB")
        config = json.loads(path.read_text(encoding="utf-8"))
        result = run(config, args.output)
    except (OSError, ValueError, TypeError) as exc:
        # Do not print the manifest, argv, or arbitrary validation values.
        print(json.dumps({"status": "refused", "reason": type(exc).__name__}), flush=True)
        return 2
    finally:
        signal.signal(signal.SIGTERM, previous)
    print(json.dumps({"task_id": result["task_id"], "status": result["status"],
                      "summary": str(Path(args.output).absolute() / "summary.json"),
                      "failed_commands": [c["id"] for c in result["commands"] if c["status"] == "failed"]}), flush=True)
    return 1 if result["status"] == "failed" else 0


if __name__ == "__main__":
    sys.exit(main())

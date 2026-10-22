# Executed HOST source-text syntax command

Executed once, cwd `/home/gadorach/Desktop/AnykaOpenCam`, at the UTC recorded in
SYNTAX_STATUS01.json. Foreground shell timeout120000ms; exit0. This is the
inspection command itself, not a callable test/fixture/helper. It never imports
the candidate, constructs transcript input, executes compiled code or writes
files. AST record metadata was lengths/names only, not joined record bytes.

```sh
python3 -B - <<'PY'
import ast, datetime, hashlib, json, sys
from pathlib import Path
rel = 'apps/qualification/encoder-query77-cold-context-v2/cold_context.py'
source = Path(rel).read_bytes()
text = source.decode('utf-8')
tree = ast.parse(text, filename=rel, mode='exec')
code = compile(text, rel, 'exec', dont_inherit=True, optimize=0)
del code
records = next(node.value for node in tree.body if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == '_RECORDS' for target in node.targets))
summary = {
    'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'method': 'Read only new module text; ast.parse then in-memory compile; discard unexecuted code object. AST metadata inspection only; no imports/calls/eval/exec of candidate, no inputs or fixtures.',
    'python': sys.version,
    'source': rel,
    'bytes': len(source),
    'sha256': hashlib.sha256(source).hexdigest(),
    'lines': len(text.splitlines()),
    'ast_parse': 'PASS',
    'in_memory_compile': 'PASS',
    'candidate_import_or_call_or_code_object_execution': False,
    'pyc_or_code_object_serialized': False,
    'records_count_from_AST': len(records.elts),
    'record_metadata_not_input': [{'name': entry.elts[0].value, 'literal_bytes': len(entry.elts[1].value)} for entry in records.elts],
    'imports_from_AST': [node.module for node in tree.body if isinstance(node, ast.ImportFrom)],
    'top_level_functions_from_AST': [node.name for node in tree.body if isinstance(node, ast.FunctionDef)],
    'classes_from_AST': [node.name for node in tree.body if isinstance(node, ast.ClassDef)],
    'call_sites_from_AST': [{'line': node.lineno, 'callee': ast.unparse(node.func)} for node in ast.walk(tree) if isinstance(node, ast.Call)]
}
print(json.dumps(summary, indent=2))
PY
```

Output metadata:19 records; per-record lengths in position order:
2,2,54,2,15,30,7,37,22,30,15,15,15,25,18,39,24,2,6.
No literal byte values or concatenated full expected input were emitted.
Callsite names/lines and their source-only closure are covered in CALLSITE_CLOSURE.md.
Full syntax status and source/compiler identity are in SYNTAX_STATUS01.json.

# Executed final HOST source-text inspection command

Executed once in canonical workspace, foreground timeout120000ms, exit0. Old AST
parsing is solely the explicitly requested source-equivalence comparison. Compile
is only of new text; no source imports, float formatter call, fixture/mock/input,
code-object execution or output-file generation. Tool output is summarized in
SYNTAX01.json/SOURCE_DELTA01.json; exact final command follows.

```sh
python3 -B - <<'PY'
from pathlib import Path
import ast,copy,difflib,hashlib,json,sys
oldname='work/lightbulb-camera/implementation-20261008T205404Z/uart_capture.py'
newname='apps/qualification/encoder-query77-recorder-timer/uart_capture.py'
oldraw=Path(oldname).read_bytes();newraw=Path(newname).read_bytes()
a=ast.parse(oldraw.decode('utf-8'),filename=oldname);b=ast.parse(newraw.decode('utf-8'),filename=newname)
code=compile(newraw.decode('utf-8'),newname,'exec',dont_inherit=True,optimize=0);del code
projected=copy.deepcopy(b)
helpers=[n for n in projected.body if isinstance(n,ast.FunctionDef) and n.name=='_native_capture_timer_status']
roots=[n for n in projected.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='ROOT' for t in n.targets)]
oldroot=next(n for n in a.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='ROOT' for t in n.targets))
capture=next(n for n in projected.body if isinstance(n,ast.FunctionDef) and n.name=='capture')
statuses=[n for n in ast.walk(capture) if isinstance(n,ast.If) and ast.unparse(n.test)=="action == 'status'"]
if len(helpers)!=1 or len(roots)!=1 or len(statuses)!=1:raise SystemExit('Unexpected authored declaration/branch shape')
status=statuses[0];gates=[n for n in status.body if isinstance(n,ast.If) and ast.unparse(n.test)=="private_capture and req.get('private_status') is True"]
if len(gates)!=1 or len(gates[0].body)!=1 or gates[0].orelse:raise SystemExit('Unexpected private timer gate shape')
gate=gates[0]
projected.body.remove(helpers[0]);projected.body[projected.body.index(roots[0])]=copy.deepcopy(oldroot);status.body.remove(gate)
equivalent=ast.dump(a,include_attributes=False)==ast.dump(projected,include_attributes=False)
olddefs={n.name:n for n in a.body if isinstance(n,ast.FunctionDef)};newdefs={n.name:n for n in b.body if isinstance(n,ast.FunctionDef)}
helper=newdefs['_native_capture_timer_status']
oldcalls=[ast.unparse(n.func) for n in ast.walk(a) if isinstance(n,ast.Call) and ast.unparse(n.func).startswith('time.')]
newcalls=[ast.unparse(n.func) for n in ast.walk(b) if isinstance(n,ast.Call) and ast.unparse(n.func).startswith('time.')]
diff=list(difflib.unified_diff(oldraw.decode().splitlines(),newraw.decode().splitlines(),fromfile=oldname,tofile=newname,lineterm=''))
receipt={'order':'FINAL_NEW_TEXT_SYNTAX_AND_STATIC_AST_DELTA_NO_CLOCK_READ','python':sys.version,'original_sha256':hashlib.sha256(oldraw).hexdigest(),'candidate_sha256':hashlib.sha256(newraw).hexdigest(),'candidate_bytes':len(newraw),'candidate_lines':len(newraw.splitlines()),'ast_parse_old_and_new':'PASS','new_unit_in_memory_compile':'PASS','compiled_units':1,'original_compiled':False,'candidate_import_call_or_code_object_execution':False,'pyc_or_code_object_serialized':False,'changed_old_top_functions':[k for k in olddefs if k in newdefs and ast.dump(olddefs[k])!=ast.dump(newdefs[k])],'unchanged_old_top_functions':[k for k in olddefs if k in newdefs and ast.dump(olddefs[k])==ast.dump(newdefs[k])],'added_top_functions':sorted(newdefs.keys()-olddefs.keys()),'project_only_ROOT_new_helper_and_one_status_gate_equals_complete_original_AST':equivalent,'gate_line':gate.lineno,'gate_test':ast.unparse(gate.test),'gate_body':ast.unparse(gate.body[0]),'helper_lines':[helper.lineno,helper.end_lineno],'helper_call_sites':[{'line':n.lineno,'callee':ast.unparse(n.func)} for n in ast.walk(helper) if isinstance(n,ast.Call)],'time_call_sites_old':len(oldcalls),'time_call_sites_new':len(newcalls),'time_call_callee_order_unchanged':oldcalls==newcalls,'assert_count_old':sum(isinstance(n,ast.Assert) for n in ast.walk(a)),'assert_count_new':sum(isinstance(n,ast.Assert) for n in ast.walk(b)),'text_diff_added_lines':sum(n.startswith('+') and not n.startswith('+++') for n in diff),'text_diff_removed_lines':sum(n.startswith('-') and not n.startswith('---') for n in diff),'text_diff_hunks':sum(n.startswith('@@') for n in diff)}
print(json.dumps(receipt,indent=2));print('FINAL_EXACT_PUBLIC_SOURCE_DIFF')
for line in diff:print(line)
if not equivalent or oldcalls!=newcalls:raise SystemExit('Unexpected source delta')
PY
```

Final reported source:340 lines/26563B,
`be0fee5d7aaa55aba5616d04689e309ee9a38255376188ede074cd5be1c7392d`.
AST projection equality true;15/15 unchanged time-call sites; two/two unchanged
asserts; +19/-1 text lines, three hunks. Interpreter Python3.14.8/GCC16.2.1 as
recorded in syntax receipt. No current clock/kernel/service/namespace query.

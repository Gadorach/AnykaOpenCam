# Additional consumer selection — author static audit only

Build01 discovered that the actual frozen sensor C unit includes handoff.h for
the original NH_OUTPUT_NS bound. It is recompiled in build02, never reused. Its
body is unchanged. `-include` selects the real complete accepted memory handoff;
that header's legitimate retained guard prevents subsequent old definitions.
The original sensor header and sc1346 table remain real frozen inputs. Compiler
feature macros are supplied before this preinclude. No fabricated guard exists.

The initial AUTHOR_AUDIT.md phrase banning *any* old header dependency path was
too strong: GCC can record an opened original handoff/adapter header even though
its body is correctly excluded by the already selected COMPLETE replacement.
Build02 permits those two path records, but still refuses any old memory_reserve
or memory_runtime header and refuses reuse of ANY consumer listing those headers,
handoff, native adapter or owned-runtime headers. Actual changed source compiling
its new memory fields also requires the complete new definitions. This is not
an old binary ABI assertion. Reused sensor is excluded rather than excused.

The one actual private native/static memory/runtime ownership and new typed API,
range/history/retention code are unchanged from build01. No production C/header
correction or runtime test follows this build-method correction. Read the final
handback/report for actual results; neither author audit is independent review.

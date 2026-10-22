# Reader-only metadata corrections

The first static reader invocation stopped at line76, before writing evidence:
`assert actual == set(files) | {str(candidate_seal)}`.
Reader SHA256 was
`a8d64ba923136046756cc81c49a416e2a0eb05acc0b40b5339544d223d901961`.
Its directory enumeration omitted the separate author report, which is correctly
included in the132-file author seal. A read-only set comparison found no unsealed
files and exactly that one omitted enumeration member. The reviewer corrected
only this reader's enumeration; no author input was changed.

Earlier interactive metadata exploration assumed the P4a excerpt JSON was a map;
it is a list. That exploration returned AttributeError and wrote no files. The
reader handles its actual list schema. Neither event was a tool/policy rejection,
candidate execution, compilation or behavioral test. Failed observations are not
described as source defects or successful verification.

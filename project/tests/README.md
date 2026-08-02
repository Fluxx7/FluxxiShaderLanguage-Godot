# FSL stress-test suite

Stress tests for the whole FSL stack (preprocessor → lexer → parser →
validator → transpiler), runnable without a GPU: each file is loaded through
`FSLFile.from_file()` in headless Godot, which parses, validates, and
transpiles but never dispatches.

**These tests target the language as designed, not as currently
implemented.** Several `valid/` files use features from the parser-overhaul
design (trailing-colon annotations, spec-constant-sized shared arrays,
break/continue/while), and every `validation_error/` test fails today because
`FSLValidator::validate_ast` is a stub. A failing test is a work item, not
necessarily a regression.

## Running

```sh
./run_fsl_tests.sh                 # everything
./run_fsl_tests.sh valid           # substring filter on the path
./run_fsl_tests.sh pe03 ve10       # multiple filters
GODOT=/path/to/godot ./run_fsl_tests.sh
FSL_TEST_TIMEOUT=40 ./run_fsl_tests.sh
```

Each file runs in its own Godot process, so a parser crash or infinite loop
(reported as `crashed`/`hung`) only kills that one test.

## Layout

- `fsl/valid/` — hard-to-parse but legal programs. Pass = no FSL error output
  **and** every kernel listed in `//! KERNELS:` produces non-empty transpiled
  source.
- `fsl/parse_error/` — structurally broken programs the **parser** must
  reject (unbalanced scopes, missing semicolons, malformed kernels/macros…).
  Pass = FSL error output appears. One defect per file so errors can't mask
  each other.
- `fsl/validation_error/` — programs that parse cleanly but are semantically
  wrong; the **validator** must reject them (type mismatches, undeclared
  names, recursion, unsized-array placement, const-ness rules, annotation
  misuse…). Pass = FSL error output appears.
- `fsl/includes/` — helper includes for the include/diamond tests; not run
  directly.
- `run_one.gd` — loads one file and prints `FSL_TEST_*` markers.
- `run_fsl_tests.sh` — driver; compares outcomes against `//! EXPECT:`.

## Test-file header convention

```
//! EXPECT: valid | parse_error | validation_error
//! KERNELS: name1 name2     (valid only: kernels that must transpile)
//! CHECK: substring         (optional: error output must contain this)
```

## How errors are detected

There is currently no structured error API on `FSLFile`; the parser reports
via `print_error` to the console. The driver treats any non-marker output
line containing `.fsl`, `Failed to load shader`, `Unexpected token`, or
`Invalid file path` as an FSL diagnostic (all parser messages include the
source file name). When `FSLFile` grows a real `get_errors()`-style API,
switch `run_one.gd` to it and delete the grep heuristic in the driver.

## Not covered (by design, for now)

- Structs, matrices, switch — not part of the current language design.
- Anything requiring a dispatch/GPU (see the water-rendering project for
  runtime testing).
- `while`/`break`/`continue` live in `v10_loop_control.fsl` only — delete it
  if those are deliberately out of scope.

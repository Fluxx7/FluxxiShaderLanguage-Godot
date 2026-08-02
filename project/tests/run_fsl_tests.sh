#!/usr/bin/env bash
# FSL language stress-test driver.
#
# Runs every .fsl under project/tests/fsl through the FSL stack in headless
# Godot (one process per file, so crashes and hangs are isolated) and checks
# the outcome against the file's `//! EXPECT:` header.
#
# Usage:
#   ./run_fsl_tests.sh                  run everything
#   ./run_fsl_tests.sh valid            run tests whose path contains "valid"
#   ./run_fsl_tests.sh pe03 ve10        any number of substring filters
#   GODOT=/path/to/godot ./run_fsl_tests.sh
#
# Header conventions inside each .fsl test file:
#   //! EXPECT: valid | parse_error | validation_error
#   //! KERNELS: name1 name2     (valid only: kernels that must transpile)
#   //! CHECK: substring         (optional: error output must contain this)

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
FSL_DIR="$SCRIPT_DIR/fsl"
GODOT="${GODOT:-/Applications/Godot.app/Contents/MacOS/Godot}"
TIMEOUT_SECS="${FSL_TEST_TIMEOUT:-20}"

if [ ! -x "$GODOT" ]; then
    echo "error: Godot binary not found at '$GODOT' (set \$GODOT to override)" >&2
    exit 2
fi

pass_count=0
fail_count=0
skip_count=0
failed_list=""

result_line() { printf '%-5s %-48s %s\n' "$1" "$2" "$3"; }

for f in $(cd "$FSL_DIR" && find . -name '*.fsl' -not -path './includes/*' | sed 's|^\./||' | sort); do
    if [ "$#" -gt 0 ]; then
        matched=0
        for pat in "$@"; do
            case "$f" in *"$pat"*) matched=1 ;; esac
        done
        [ "$matched" -eq 1 ] || continue
    fi

    file_path="$FSL_DIR/$f"
    expect=$(sed -n 's|^//! EXPECT:[[:space:]]*||p' "$file_path" | head -n1 | tr -d '\r')
    kernels=$(sed -n 's|^//! KERNELS:[[:space:]]*||p' "$file_path" | head -n1 | tr -d '\r')
    if [ -z "$expect" ]; then
        result_line SKIP "$f" "(no //! EXPECT: header)"
        skip_count=$((skip_count + 1))
        continue
    fi

    res_path="res://tests/fsl/$f"
    # perl sets an alarm that survives exec, killing hung runs with SIGALRM.
    # shellcheck disable=SC2086  # word-splitting of $kernels is intentional
    output=$(perl -e 'alarm shift @ARGV; exec @ARGV' "$TIMEOUT_SECS" \
        "$GODOT" --headless --path "$PROJECT_DIR" -s res://tests/run_one.gd -- \
        "$res_path" $kernels 2>&1)
    status=$?

    # FSL diagnostics: anything naming a .fsl file or a known loader message,
    # excluding our own FSL_TEST_ markers. Tighten this when FSLFile grows a
    # structured error API.
    errors=$(printf '%s\n' "$output" | grep -v '^FSL_TEST_' \
        | grep -E '\.fsl|Failed to load shader|Unexpected token|Invalid file path|Error found in AST')

    verdict=""
    reason=""
    if [ "$status" -ge 128 ]; then
        verdict=FAIL
        reason="(crashed: signal $((status - 128)))"
        [ "$status" -eq 142 ] && reason="(hung: killed after ${TIMEOUT_SECS}s)"
    elif ! printf '%s\n' "$output" | grep -q '^FSL_TEST_END'; then
        verdict=FAIL
        reason="(died before completing, exit $status)"
    else
        case "$expect" in
            valid)
                if [ -n "$errors" ]; then
                    verdict=FAIL; reason="(errors reported for valid file)"
                elif printf '%s\n' "$output" | grep -q '^FSL_TEST_KERNEL_MISSING'; then
                    verdict=FAIL; reason="(kernel failed to transpile)"
                else
                    verdict=PASS
                fi
                ;;
            parse_error|validation_error)
                if [ -z "$errors" ]; then
                    verdict=FAIL; reason="(invalid file accepted without errors)"
                else
                    verdict=PASS
                    while IFS= read -r want; do
                        [ -z "$want" ] && continue
                        if ! printf '%s\n' "$output" | grep -qF "$want"; then
                            verdict=FAIL; reason="(error output missing: '$want')"
                        fi
                    done <<EOF
$(sed -n 's|^//! CHECK:[[:space:]]*||p' "$file_path" | tr -d '\r')
EOF
                fi
                ;;
            *)
                verdict=SKIP; reason="(unknown EXPECT '$expect')"
                ;;
        esac
    fi

    case "$verdict" in
        PASS)
            pass_count=$((pass_count + 1))
            result_line PASS "$f" ""
            ;;
        SKIP)
            skip_count=$((skip_count + 1))
            result_line SKIP "$f" "$reason"
            ;;
        *)
            fail_count=$((fail_count + 1))
            failed_list="${failed_list}  $f $reason
"
            result_line FAIL "$f" "$reason"
            printf '%s\n' "$output" | grep -v '^ *$' | tail -n 20 | sed 's/^/      | /'
            ;;
    esac
done

echo
echo "passed: $pass_count   failed: $fail_count   skipped: $skip_count"
if [ "$fail_count" -gt 0 ]; then
    echo
    echo "failures:"
    printf '%s' "$failed_list"
    exit 1
fi

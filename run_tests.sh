#!/usr/bin/env bash
# group-105
# Automated test runner: feeds each .in file in tests/ to acaderp and compares output with .out
set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$SCRIPT_DIR/src/acaderp"

# Build the binary automatically if it has not been compiled yet
if [ ! -x "$BIN" ]; then
    make -C "$SCRIPT_DIR/src" || exit 1
fi

# Human-readable summary for each test case from the project specification
desc_for() {
    case "$1" in
        sample) echo "Worked sample from Section 4" ;;
        T01) echo "E_GRADE_TYPE (Letter grade to SU; S to REGULAR)" ;;
        T02) echo "E_BAD_GRADE (Grade a, A+, or Z)" ;;
        T03) echo "E_NOT_REGISTERED (Grade for unregistered term)" ;;
        T04) echo "E_ALREADY_GRADED (Second grade on same registration)" ;;
        T05) echo "E_THESIS_STATE (CONVERT before S; CONVERT after X)" ;;
        T06) echo "Accepted; then E_ALREADY_GRADED (S -> CONVERT B -> CONVERT again)" ;;
        T07) echo "Allowed (Thesis graded X, then registered again)" ;;
        T08) echo "8.13, not 8.12 (3 credits A + 5 credits B-)" ;;
        T09) echo "8.67 (Three 1-credit courses A, B, B)" ;;
        T10) echo "5.00 (4 credits A + 4 credits F)" ;;
        T11) echo "E_NO_GPA (Only SU courses, or only ungraded thesis)" ;;
        T12) echo "Only B counts (F, then repeated with B)" ;;
        T13) echo "Averages A- and F (GPA 8.50 and GPA 1.99)" ;;
        T14) echo "YES (Every graduation rule met exactly at limit)" ;;
        T15) echo "Credits earned; GRAD reports R4 (Thesis converted to D)" ;;
        T16) echo "All listed in order, e.g. NO R1, R3, R5" ;;
        T17) echo "NO E4 (Prerequisite failed F or not yet graded)" ;;
        T18) echo "YES, then NO E5 (Load at max, then 1 credit over)" ;;
        T19) echo "NO E6 (Thesis with earned credits one short)" ;;
        T20) echo "E_BAD_COURSE (Self prerequisite; regular course 0 credits)" ;;
        T21) echo "E_FULL (101st student; 51st course)" ;;
        T22) echo "E_PARSE (11-char roll; unknown command; term x)" ;;
        T23) echo "Later lines still run (Error in middle of file)" ;;
        T24) echo "Bonus TRANSCRIPT <roll> in term order with grade and credits" ;;
        T25) echo "Bonus SAVE <file> and LOAD <file> with atomic error rollback" ;;
        *)   echo "" ;;
    esac
}

passed=0
total=0

# Loop through tests/sample.in and tests/T01.in .. tests/T23.in
for infile in "$SCRIPT_DIR"/tests/sample.in "$SCRIPT_DIR"/tests/T*.in; do
    [ -e "$infile" ] || continue
    base="${infile%.in}"
    outfile="$base.out"
    name="$(basename "$base")"
    total=$((total + 1))

    # Run acaderp with the test input and read the expected output
    actual="$("$BIN" < "$infile")"
    expected="$(cat "$outfile")"

    echo "=========================================="
    echo "Test: $name | Spec Expected: $(desc_for "$name")"
    echo "--- Expected (.out) ---"
    printf "%s\n" "$expected"
    echo "--- Actual (acaderp) ---"
    printf "%s\n" "$actual"

    # Check for an exact match
    if [ "$actual" = "$expected" ]; then
        echo "Result: PASS"
        passed=$((passed + 1))
    else
        echo "Result: FAIL"
        diff -u "$outfile" <(printf "%s\n" "$actual") || true
    fi
done

echo "=========================================="
echo "Passed $passed / $total tests."
[ "$passed" -eq "$total" ]

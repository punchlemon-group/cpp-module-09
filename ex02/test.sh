#!/bin/bash

CMD=./PmergeMe

# temp file to record failing cases (command + output)
FAILURES_FILE=$(mktemp /tmp/pmerge_failures.XXXXXX)
trap 'if [ -s "$FAILURES_FILE" ]; then echo "Failures logged to: $FAILURES_FILE"; else rm -f "$FAILURES_FILE"; fi' EXIT

# ANSI color helpers
GREEN="\033[32m"
RED="\033[31m"
RESET="\033[0m"

# ==============================================================================
# 1. 静的テストケース (ワーストケースのチェック)
# ==============================================================================
# 形式: "LIMIT" "引数..."
static_suites=(
    # N=21, 二分探索で「大きい方」を選ぶ実装でのワーストケース (比較 66回)
    "66 21 1 13 2 17 3 12 4 20 5 15 6 19 7 14 8 18 9 16 10 11"
    # N=21, 二分探索で「小さい方」を選ぶ実装でのワーストケース (比較 66回)
    "66 1 21 9 20 5 19 10 18 2 17 7 16 3 15 8 14 4 13 6 12 11"
    "13 3 7 2 5 1 4 6"
)

# ==============================================================================
# 2. ランダムテストケース (既存のスイート)
# ==============================================================================
# 形式: "RUNS NUMBERCOUNT LIMIT"
random_suites=(
    "10 1 0"
    "10 2 1"
    "10 3 3"
    "10 4 5"
    "10 5 7"
    "20 6 10"
    "30 7 13"
    "30 8 16"
    "40 9 19"
    "50 10 22"
    "100 100 534"
    "100 1000 8641"
    "100 10000 119085"
)


if [ ! -x "$CMD" ]; then
    echo "Error: $CMD not found or not executable. Please build it first."
    exit 1
fi

# --- 共通テスト関数 ---
# $1: 期待する比較回数の上限 (LIMIT)
# $2: PmergeMeに渡す引数列 (文字列)
run_test () {
    local LIMIT="$1"
    local ARGS="$2"
    local N=$(echo "$ARGS" | wc -w) # 引数の数をカウント

    OUTPUT=$($CMD $ARGS)

    # 1. ソートされているかチェック
    if echo "$OUTPUT" | grep -qF "NOT SORTED"; then
        # record failure
        printf "--- FAILURE ---\n" >> "$FAILURES_FILE"
        printf "ARGS: %s\n" "$ARGS" >> "$FAILURES_FILE"
        printf "REASON: NOT SORTED (N=%d)\n" "$N" >> "$FAILURES_FILE"
        printf "OUTPUT:\n%s\n\n" "$OUTPUT" >> "$FAILURES_FILE"
        # red dot
        printf "%b.%b" "$RED" "$RESET"
        return 1
    fi

    # 2. 比較回数をパース
    COMP=$(echo "$OUTPUT" | grep "Number of comparisons" | grep -m 1 "" | awk '{print $4}')

    if [ -z "$COMP" ] || ! [[ "$COMP" =~ ^[0-9]+$ ]]; then
        printf "--- FAILURE ---\n" >> "$FAILURES_FILE"
        printf "ARGS: %s\n" "$ARGS" >> "$FAILURES_FILE"
        printf "REASON: Number of comparisons not found or invalid\n" >> "$FAILURES_FILE"
        printf "OUTPUT:\n%s\n\n" "$OUTPUT" >> "$FAILURES_FILE"
        printf "%b.%b" "$RED" "$RESET"
        return 1
    fi

    # 3. 比較回数をチェック
    if [ "$COMP" -gt "$LIMIT" ]; then
        printf "--- FAILURE ---\n" >> "$FAILURES_FILE"
        printf "ARGS: %s\n" "$ARGS" >> "$FAILURES_FILE"
        printf "REASON: comparisons = %s > %s\n" "$COMP" "$LIMIT" >> "$FAILURES_FILE"
        printf "OUTPUT:\n%s\n\n" "$OUTPUT" >> "$FAILURES_FILE"
        printf "%b.%b" "$RED" "$RESET"
        return 1
    fi

    # success: green dot
    printf "%b.%b" "$GREEN" "$RESET"
    return 0
}
# --- ここまで共通テスト関数 ---


# === 1. 静的テストの実行 ===
echo "=== Running Static Worst-Case Tests ==="
for suite in "${static_suites[@]}"; do
    # "LIMIT" と "ARGS..." を分離
    read -r LIMIT ARGS <<< "$suite"
    run_test "$LIMIT" "$ARGS"
done
echo
echo "" # newline after dot stream
if [ -s "$FAILURES_FILE" ]; then
    echo "Some static tests failed. See failures below."
else
    echo "✅ Static tests passed."
fi
echo


# === 2. ランダムテストの実行 ===
echo "=== Running Random Suites ==="
for suite in "${random_suites[@]}"; do
    read -r RUNS NUMBERCOUNT LIMIT <<< "$suite"
    echo "=== Test: NUMBERCOUNT=$NUMBERCOUNT, RUNS=$RUNS, LIMIT=$LIMIT ==="
    for i in $(seq 1 "$RUNS"); do
        # ランダムな引数列を生成
        RANDOM_ARGS=$(shuf -i 1-100000 -n $NUMBERCOUNT | tr "\n" " ")
        run_test "$LIMIT" "$RANDOM_ARGS"
    done
    echo
    if [ -s "$FAILURES_FILE" ]; then
        echo "Some runs in this suite failed (see failures below)."
    else
        echo "✅ Suite passed (N=$NUMBERCOUNT) — all $RUNS runs comparisons <= $LIMIT"
    fi
    echo
done

if [ -s "$FAILURES_FILE" ]; then
    echo
    echo "=== Failures ==="
    cat "$FAILURES_FILE"
    echo
    echo "Total failures logged to: $FAILURES_FILE"
    exit 1
else
    echo "🎉 All suites passed."
    # remove temp file if empty
    rm -f "$FAILURES_FILE"
    exit 0
fi

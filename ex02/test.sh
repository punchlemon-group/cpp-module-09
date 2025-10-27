#!/bin/bash

# PmergeMeの実行ファイル
CMD=./PmergeMe
# Python (python3 or python) が必要です
PYTHON_CMD=$(command -v python3 || command -v python)

# 失敗ケースを記録する一時ファイル
FAILURES_FILE=$(mktemp /tmp/pmerge_failures.XXXXXX)

# スクリプト終了時に失敗ログファイルが空でなければパスを表示、空なら削除
trap 'if [ -s "$FAILURES_FILE" ]; then echo "Failures logged to: $FAILURES_FILE"; else rm -f "$FAILURES_FILE"; fi' EXIT

# Note: we no longer save worst-case sequences to a file; they are shown on stdout only.

# ANSI color helpers (use $'..' so escapes are actual bytes)
GREEN=$'\033[32m'
RED=$'\033[31m'
RESET=$'\033[0m'
YELLOW=$'\033[33m'

# Dot-wrapping: avoid extremely long single lines of dots which cause wrapping issues.
DOT_WRAP=80
DOT_COL=0

# Print a green dot to stderr and wrap line after DOT_WRAP
print_dot_green() {
    printf "%b.%b" "$GREEN" "$RESET" >&2
    DOT_COL=$((DOT_COL+1))
    if [ "$DOT_COL" -ge "$DOT_WRAP" ]; then
        printf "\n" >&2
        DOT_COL=0
    fi
}

# Print a red dot to stderr and wrap line after DOT_WRAP
print_dot_red() {
    printf "%b.%b" "$RED" "$RESET" >&2
    DOT_COL=$((DOT_COL+1))
    if [ "$DOT_COL" -ge "$DOT_WRAP" ]; then
        printf "\n" >&2
        DOT_COL=0
    fi
}
# ==============================================================================
# 1. 静的テストケース (既知のワーストケース)
# ==============================================================================
# Nが大きく全探索が不可能な、既知のワーストケースをここに残します。
# 形式: "LIMIT" "引数..."
static_suites=(
    "66 21 1 13 2 17 3 12 4 20 5 15 6 19 7 14 8 18 9 16 10 11"
)

# ==============================================================================
# 2. 探索的テストケース (自動探索)
# ==============================================================================
# 形式: "RUNS NUMBERCOUNT LIMIT"
# RUNS="ALL": N <= 8 の場合、全順列(N!)をテストし、最大比較回数がLIMIT以下か検証。
# RUNS=<数値>: N > 8 の場合、<数値>回ランダムテストを実行。
exploratory_suites=(
    "ALL 1 0"
    "ALL 2 1"
    "ALL 3 3"
    "ALL 4 5"
    "ALL 5 7"
    "ALL 6 10"
    
    # Nが大きくなるとランダムサンプリングに戻す
    "100 100 534"
    "100 1000 8641"
    "100 10000 119085"
)


# ==============================================================================
# Edge-case tests for argument handling
# Format: "EXPECTED|ARGS" where EXPECTED is 'ok' or 'err'. ARGS may be empty for no-args test.
edge_suites=(
    # Basic expectations
    "err|"               # no args -> expect an error or graceful message
    "ok|1"               # single positive value should be handled
    "err|0"              # zero is considered invalid (observed behavior)

    # malformed inputs
    "err|a b"            # non-numeric tokens
    "err|1a 2"           # mixed alphanumeric
    "err|1.5 2"          # float not integer

    # sign / range
    "err|-1 2"           # negative number
    "err|2147483648"     # too-large integer (overflow-like)

    # duplicates and small sequences
    "ok|1 1 1"           # duplicates
    "ok|1 2"             # small valid sequence
    "ok|2 3 5 7"         # longer valid sequence
)


if [ ! -x "$CMD" ]; then
    echo "Error: $CMD not found or not executable. Please build it first."
    exit 1
fi

if [ -z "$PYTHON_CMD" ]; then
    echo "Error: python3 or python command not found. This script requires python for 'ALL' suites."
    exit 1
fi

# --- 共通テスト関数 ---
# $1: 期待する比較回数の上限 (LIMIT)
# $2: PmergeMeに渡す引数列 (文字列)
# (変更) 成功時は比較回数を stdout に出力し、リターンコード 0 を返す
# (変更) 失敗時はエラーを FAILURES_FILE に記録し、stdout には何も出力せず、リターンコード 1 を返す
run_test () {
    local LIMIT="$1"
    local ARGS="$2"
    # (削除) $3 (MAX_COMP_VAR_NAME)
    # (削除) $4 (MAX_COMP_ARGS_VAR_NAME)
    local N=$(echo "$ARGS" | wc -w) # 引数の数をカウント

    OUTPUT=$($CMD $ARGS 2>&1) # stderr もキャプチャ (エラーケースのため)

    # 1. エラーをチェック (Errorという文字列があったら失敗)
    if echo "$OUTPUT" | grep -qE "Error|ERROR|error"; then
        # log failure (use here-doc to avoid printf quirks)
        cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: Unexpected error output (N=$N)
OUTPUT:
$OUTPUT

EOF
            # red dot to stderr
            print_dot_red
        return 1
    fi

    # 2. ソートされているかチェック
    if echo "$OUTPUT" | grep -qF "NOT SORTED"; then
    cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: NOT SORTED (N=$N)
OUTPUT:
$OUTPUT

EOF
    print_dot_red
        return 1
    fi

    # 3. 比較回数をパース
    COMP=$(echo "$OUTPUT" | grep "Number of comparisons" | grep -m 1 "" | awk '{print $4}')

    if [ -z "$COMP" ] || ! [[ "$COMP" =~ ^[0-9]+$ ]]; then
    cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: Number of comparisons not found or invalid
OUTPUT:
$OUTPUT

EOF
    print_dot_red
        return 1
    fi
    
    # 4. (削除) 最大比較回数を更新するロジックは呼び出し元へ

    # 5. 上限(LIMIT)をチェック
    if [ "$COMP" -gt "$LIMIT" ]; then
    cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: comparisons = $COMP > $LIMIT
OUTPUT:
$OUTPUT

EOF
    print_dot_red
        return 1
    fi

    # 成功: print green dot to stderr (no newline) and COMP to stdout
    print_dot_green
    echo "$COMP" # (変更) 比較回数を stdout に出力
    return 0
}
# --- ここまで共通テスト関数 ---

# --- エッジケース用テスト関数 ---
# $1: expected ('ok' or 'err')
# $2: args (may be empty)
run_edge_test() {
    local EXPECTED="$1"
    local ARGS="$2"
    # execute and capture
    OUTPUT=$($CMD $ARGS 2>&1)
    RC=$?

    if [ "$EXPECTED" = "err" ]; then
        # expect an error
        if [ $RC -ne 0 ] || echo "$OUTPUT" | grep -qE "Error|ERROR|error"; then
            # expected error: print green dot to stderr
            print_dot_green
            return 0
        else
            # unexpected success: print red dot to stderr
            print_dot_red
            cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: expected error but program succeeded
OUTPUT:
$OUTPUT

EOF
            return 1
        fi
    else
        # expect success
        if [ $RC -ne 0 ] || echo "$OUTPUT" | grep -qE "Error|ERROR|error|NOT SORTED"; then
            # expected success but got error: print red dot to stderr
            print_dot_red
            cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: expected success but got error or NOT SORTED
OUTPUT:
$OUTPUT

EOF
            return 1
        fi
        if ! echo "$OUTPUT" | grep -q "Number of comparisons"; then
            # missing comparisons: print red dot to stderr
            print_dot_red
            cat >> "$FAILURES_FILE" <<EOF
--- FAILURE ---
ARGS: $ARGS
REASON: missing 'Number of comparisons' in output
OUTPUT:
$OUTPUT

EOF
            return 1
        fi
        print_dot_green
        return 0
    fi
}


# グローバルな失敗フラグ
GLOBAL_FAIL=0

# === 1. 静的テストの実行 ===
echo "=== Running Static Worst-Case Tests ==="
for suite in "${static_suites[@]}"; do
    read -r LIMIT ARGS <<< "$suite"
    N=$(echo $ARGS | wc -w)

    # print prompt to stderr so dots (also on stderr) line up
    echo -n "Run test (N=$(printf "%6d" "$N"))... " >&2
    # break line here so the dot appears on the next line (user requested)
    printf "\n" >&2
    DOT_COL=0

    # run_test prints a colored dot to stderr and outputs COMP to stdout on success
    COMP_OUTPUT=$(run_test "$LIMIT" "$ARGS")
    RC=$?
    # finish the dot line and reset dot column so subsequent summaries don't continue the same line
    printf "\n" >&2
    DOT_COL=0

    if [ $RC -ne 0 ]; then
        GLOBAL_FAIL=1
        # report a consistent per-case failure summary to stderr
        printf "%b❌ Static test FAILED (N=%d, LIMIT=%d) — see failures in %s%b\n" "$RED" "$N" "$LIMIT" "$FAILURES_FILE" "$RESET" >&2
    else
        # success: COMP_OUTPUT contains the comparisons count
        COMP=$COMP_OUTPUT
        printf "%bMax comparisons observed: %d%b\n" "$YELLOW" "$COMP" "$RESET" >&2
        printf "%b✅ Static test passed (N=%d) — comparisons: %d%b\n" "$GREEN" "$N" "$COMP" "$RESET" >&2
    fi
done
echo >&2

# (edge-case tests moved below to run after exploratory suites)


# === 2. 探索的テストの実行 ===
echo "=== Running Exploratory Suites ==="
for suite in "${exploratory_suites[@]}"; do
    read -r RUNS NUMBERCOUNT LIMIT <<< "$suite"
    echo "=== Test: NUMBERCOUNT=$NUMBERCOUNT, RUNS=$RUNS, LIMIT=$LIMIT ==="
    
    # 観測された最大の比較回数
    max_observed_comp=0
    max_comp_args="" # 追加: 最大比較回数を与えた引数列 (改行区切りで複数保持)
    # 失敗したランのカウント
    fail_count=0

    if [ "$RUNS" = "ALL" ]; then
        # --- 全順列テスト ---
        if [ "$NUMBERCOUNT" -gt 8 ]; then # 8までに制限
            echo "${YELLOW}Warning: N=$NUMBERCOUNT is too large for 'ALL' (limit N<=8). Skipping.${RESET}"
            continue
        fi
        
        # Pythonスクリプトを定義 (1からNまでの全順列を生成)
        PY_SCRIPT="
import itertools, sys
try:
    N = int(sys.argv[1])
    # 1からNまでの順列を生成
    perms = itertools.permutations(range(1, N + 1))
    for p in perms:
        # スペース区切りで出力
        print(' '.join(map(str, p)))
except (IOError, BrokenPipeError):
    # パイプが(headやエラーで)閉じられた場合のエラーを無視
    sys.stderr.close()
    pass
except Exception as e:
    # その他のPythonエラー
    print(f\"Python error: {e}\", file=sys.stderr)
    pass
"
        
        echo "Running full permutation test (N=$NUMBERCOUNT)..."
        # Pythonスクリプトを実行し、出力を1行ずつ読み込む
        # (重要) プロセス置換 < <(...) を使用し、whileループを現在のシェルで実行
        while read -r ARGS; do
            # ARGSが空でないことを確認
            if [ -z "$ARGS" ]; then continue; fi
            
            # (変更) run_test を呼び出し、stdout (COMP) をキャプチャ
            # stderr のドットはそのままコンソールに出力される
            COMP_OUTPUT=$(run_test "$LIMIT" "$ARGS")
            
            # (変更) run_test のリターンコードをチェック
            if [ $? -ne 0 ]; then
                fail_count=$((fail_count + 1))
                # 失敗(LIMIT超過など)があっても、ワーストケース探索のために続行
            else
                # 成功した場合のみ、COMP_OUTPUT (比較回数) を処理
                COMP=$COMP_OUTPUT
                
                # (変更) 最大比較回数のロジックをここに移動
                if [ "$COMP" -gt "$max_observed_comp" ]; then
                    max_observed_comp=$COMP
                    max_comp_args="$ARGS" # 上書き
                elif [ "$COMP" -eq "$max_observed_comp" ]; then
                    # N=1 (Comp=0) の場合、または Comp=0 ではない最大値の場合
                    if [ "$NUMBERCOUNT" -eq 1 ] || [ "$max_observed_comp" -ne 0 ]; then
                        if [ -z "$max_comp_args" ]; then
                            max_comp_args="$ARGS"
                        else
                            # (変更) 安全な printf で改行追加
                            max_comp_args=$(printf "%s\n%s" "$max_comp_args" "$ARGS")
                        fi
                    fi
                fi
            fi
            # (削除) run_test がドットを出力するようになった
    done < <("$PYTHON_CMD" -c "$PY_SCRIPT" "$NUMBERCOUNT")
    # finish the dot line (dots are printed to stderr by run_test)
    printf "\n" >&2
        
    # (変更) 全探索の場合、見つかった最悪ケース(複数)はファイルに保存せず、stdoutにも出力しません
    # max_comp_args remains available internally for diagnostics but will not be printed
    else
        # --- ランダムテスト ---
        echo "Running $RUNS random tests..."
    for i in $(seq 1 "$RUNS"); do
            RANDOM_ARGS=$(shuf -i 1-100000 -n $NUMBERCOUNT | tr "\n" " ")
            
            # (変更) run_test を呼び出し、COMP をキャプチャ
            COMP_OUTPUT=$(run_test "$LIMIT" "$RANDOM_ARGS")
            
            if [ $? -ne 0 ]; then
                fail_count=$((fail_count + 1))
            else
                # 成功した場合のみ、max_observed_comp を更新
                COMP=$COMP_OUTPUT
                if [ "$COMP" -gt "$max_observed_comp" ]; then
                    max_observed_comp=$COMP
                fi
            fi
             # (削除) run_test がドットを出力するようになった
        done
        # finish the dot line (dots are printed to stderr by run_test)
        printf "\n" >&2
    fi
    # --- スイート結果のサマリー ---
    # Print the observed maximum in a consistent color and on its own line.
    printf "%bMax comparisons observed: %d%b\n" "$YELLOW" "$max_observed_comp" "$RESET" >&2

    if [ "$max_observed_comp" -gt "$LIMIT" ]; then
        printf "%b❌ Suite FAILED: Max observed (%d) > LIMIT (%d)%b\n" "$RED" "$max_observed_comp" "$LIMIT" "$RESET" >&2
        GLOBAL_FAIL=1
    elif [ "$fail_count" -gt 0 ]; then
        # LIMIT違反以外の失敗（ソートされていない、パースエラーなど）
        printf "%b❌ Suite FAILED: %d runs failed (e.g., NOT SORTED or parse error).%b\n" "$RED" "$fail_count" "$RESET" >&2
        GLOBAL_FAIL=1
    else
        # 成功（全探索/ランダム問わず同じフォーマット）
        printf "%b✅ Suite passed (N=%s) — all %s runs comparisons <= %d (Max observed: %d)%b\n" "$GREEN" "$NUMBERCOUNT" "$RUNS" "$LIMIT" "$max_observed_comp" "$RESET" >&2
    fi
    echo
done


# === 3. エッジケーステスト ===
echo "=== Running Edge-case Tests ==="
edge_fail_count=0
for entry in "${edge_suites[@]}"; do
    expected="${entry%%|*}"
    args="${entry#*|}"
    # show a short label for empty-args
    if [ -z "$args" ]; then
        label="(no args)"
    else
        label="$args"
    fi
    run_edge_test "$expected" "$args"
    if [ $? -ne 0 ]; then
        # do not print failure reason inline; the failure is logged to $FAILURES_FILE
        GLOBAL_FAIL=1
        edge_fail_count=$((edge_fail_count+1))
    fi
done
echo
if [ $edge_fail_count -eq 0 ]; then
    echo "✅ Edge-case tests passed."
else
    echo "❌ Edge-case tests: $edge_fail_count failures (details in $FAILURES_FILE)."
fi
echo

# === 最終結果 ===
if [ "$GLOBAL_FAIL" -ne 0 ] || [ -s "$FAILURES_FILE" ]; then
    echo
    echo "=== Failures ==="
    cat "$FAILURES_FILE"
    echo
    echo "Total failures logged to: $FAILURES_FILE"
    exit 1
else
    echo "🎉 All suites passed."
    # (note) worst-case sequences are not saved to a file; failures (if any) remain logged.
    # 成功したら一時ファイルを削除
    rm -f "$FAILURES_FILE"
    exit 0
fi


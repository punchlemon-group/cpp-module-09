#!/bin/bash

CMD=./PmergeMe

suites=(
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

for suite in "${suites[@]}"; do
    read -r RUNS NUMBERCOUNT LIMIT <<< "$suite"
    echo "=== Test: NUMBERCOUNT=$NUMBERCOUNT, RUNS=$RUNS, LIMIT=$LIMIT ==="
    for i in $(seq 1 "$RUNS"); do
        echo "Run $i..."
        OUTPUT=$($CMD $(shuf -i 1-100000 -n $NUMBERCOUNT | tr "\n" " "))
        if echo "$OUTPUT" | grep -qF "NOT SORTED"; then
            echo "❌ Test failed (N=$NUMBERCOUNT): NOT SORTED"
            exit 1
        fi
        COMP=$(echo "$OUTPUT" | grep "Number of comparisons" | head -1 | awk '{print $4}')
        if [ -z "$COMP" ] || ! [[ "$COMP" =~ ^[0-9]+$ ]]; then
            echo "Error: Number of comparisons not found. Please output it."
            exit 1
        fi
        echo "Number of comparisons: $COMP"
        if [ "$COMP" -gt "$LIMIT" ]; then
            echo "❌ Test failed (N=$NUMBERCOUNT): comparisons = $COMP > $LIMIT"
            exit 1
        fi
    done
    echo "✅ Suite passed (N=$NUMBERCOUNT) — all $RUNS runs comparisons <= $LIMIT"
    echo
done

echo "🎉 All suites passed."
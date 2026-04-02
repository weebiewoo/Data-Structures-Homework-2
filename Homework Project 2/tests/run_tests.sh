#!/bin/bash

set -e

echo "Building simplify..."
make

echo "Running tests..."

run_test() {
    input_file=$1
    target=$2
    expected_file=$3
    actual_file="test_cases/actual_output.tmp"

    ./simplify "test_cases/$input_file" "$target" > "$actual_file"

    if diff -u "$actual_file" "test_cases/$expected_file" > /dev/null; then
        echo "PASS: $input_file"
    else
        echo "FAIL: $input_file"
        diff -u "$actual_file" "test_cases/$expected_file" || true
    fi
}

run_test "input_blob_with_two_holes.csv" 17 "output_blob_with_two_holes.txt"
run_test "input_cushion_with_hexagonal_hole.csv" 13 "output_cushion_with_hexagonal_hole.txt"
run_test "input_lake_with_two_islands.csv" 17 "output_lake_with_two_islands.txt"
run_test "input_original_01.csv" 99 "output_original_01.txt"
run_test "input_original_02.csv" 99 "output_original_02.txt"
run_test "input_original_03.csv" 99 "output_original_03.txt"
run_test "input_original_04.csv" 99 "output_original_04.txt"
run_test "input_original_05.csv" 99 "output_original_05.txt"
run_test "input_original_06.csv" 99 "output_original_06.txt"
run_test "input_original_07.csv" 99 "output_original_07.txt"
run_test "input_original_08.csv" 99 "output_original_08.txt"
run_test "input_original_09.csv" 99 "output_original_09.txt"
run_test "input_original_10.csv" 99 "output_original_10.txt"
run_test "input_rectangle_with_two_holes.csv" 7 "output_rectangle_with_two_holes.txt"
run_test "input_wavy_with_three_holes.csv" 21 "output_wavy_with_three_holes.txt"

rm -f test_cases/actual_output.tmp

echo "All tests completed."
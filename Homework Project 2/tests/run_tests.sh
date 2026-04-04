#!/bin/bash

set -e

echo "Building simplify..."
make

echo "Running tests..."

run_test() {
    input_file=$1
    target=$2
    output_file=$3

    actual_file="output/actual_output.tmp"

    ./simplify "test_cases/$input_file" "$target" > "output/$output_file"

    # if diff -u "output/$actual_file" "test_cases/$expected_file" > /dev/null; then
    #     echo "PASS: $input_file"
    # else
    #     echo "FAIL: $input_file"
    #     diff -u "output/$actual_file" "test_cases/$expected_file" || true
    # fi
}

run_test "input_blob_with_two_holes.csv" 17 "output_blob_with_two_holes.txt"
run_test "input_cushion_with_hexagonal_hole.csv" 13 "output_cushion_with_hexagonal_hole.txt"
run_test "input_lake_with_two_islands.csv" 17 "output_lake_with_two_islands.txt"
run_test "input_rectangle_with_two_holes.csv" 7 "output_rectangle_with_two_holes.txt"
run_test "input_wavy_with_three_holes.csv" 21 "output_wavy_with_three_holes.txt"
run_test "input_original_01.csv" 99 "output_original_01.txt"
run_test "input_jaggedlake.csv" 99 "output_jaggedlake.txt"
run_test "input_realsmoothlake.csv" 99 "output_realsmoothlake.txt"
run_test "input_smoothlake.csv" 99 "output_smoothlake.txt"
run_test "input_winkwonklake.csv" 99 "output_winkwonklake.txt"

# rm -f output/actual_output.tmp

echo "All tests completed."
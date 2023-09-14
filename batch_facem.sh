#!/bin/bash

# Path to the directory containing the .vrs files
vrs_directory="/workspace/chenjoya/data/vrs/"

# Use the find command to locate all .vrs files recursively under vrs_directory
find "$vrs_directory" -type f -name "*.vrs" | while read -r vrs_file; do
    # Extract filename without path and extension
    filename=$(basename -- "$vrs_file")
    no_extension="${filename%.*}"

    # Output filename with _facem suffix
    output_filename="${no_extension}_facem.vrs"

    # Extract directory of the current .vrs file
    vrs_subdir=$(dirname -- "$vrs_file")

    # Full path to the output file
    output_path="$vrs_subdir/$output_filename"

    # Execute your specific command
    ./vrs/build/tools/vrs/vrs copy --facem-vrs "$vrs_file" --to "$output_path"

    # Check the exit code of the last command to make sure it succeeded
    if [ $? -eq 0 ]; then
        echo "Successfully processed $vrs_file -> $output_path"
    else
        echo "Failed to process $vrs_file"
    fi
done

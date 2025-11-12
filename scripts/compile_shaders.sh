#!/bin/bash

# Accept input and output directories as parameters
inputDir="${1:-assets/vulkan/shaders}"
outputDir="${2:-assets/vulkan/shaders/bin}"

# Create output directory if it doesn't exist
mkdir -p "$outputDir"

# Define shader file extensions to compile
extensions=("vert" "frag" "comp" "geom" "tesc" "tese")

# Set a counter for modified files
compiled_count=0
skipped_count=0

for ext in "${extensions[@]}"; do
    for file in "$inputDir"/*."$ext"; do
        [ -e "$file" ] || continue
        filename=$(basename "$file")
        base="${filename%.*}"
        outputFile="$outputDir/${base}.${ext}.spv"
        
        # Check if compiled file exists and is newer than source
        if [ -f "$outputFile" ] && [ "$file" -ot "$outputFile" ]; then
            echo "Skipping $file (already up to date)"
            ((skipped_count++))
        else
            echo "Compiling $file -> $outputFile"
            glslc "$file" -o "$outputFile"
            if [ $? -eq 0 ]; then
                ((compiled_count++))
            fi
        fi
    done
done

echo "Compilation complete: $compiled_count file(s) compiled, $skipped_count file(s) skipped."
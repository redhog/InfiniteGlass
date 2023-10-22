#!/bin/bash

# Find all the SVG files to process
# files=$(find . -type f -name "*.svg")
files=$(find . -type d \( -name fontawesome* \) -prune -o -type f -name *.svg -print)

for file in $files; do
  echo "Processing $file..."

  tempfile="$(mktemp).svg"

  # Sanitize SVG using Inkscape commands
  inkscape "$file" --export-plain-svg --export-type=svg --export-filename="$tempfile"
  inkscape "$tempfile" --verb=EditSelectAll --verb=SelectionSimplify --verb=FileSave --verb=FileQuit

  mv -f "$tempfile" "$file"
done

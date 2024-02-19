# Extracts C function names  from luajit's ffi.cdef section.
# output function names in "module definition file" style,
# i.e def file that can be passed to linker to export the symbols

# This script accepts lua file path's as (unnamed) parameters.

$functionNames=@()
$ffiSnippetPattern= "(?sm)ffi\.cdef\[\[(.*?)\]\]"
$funcDeclarationPattern = '(?ms)(?<=\n|^)\s*(?!typedef)(?:\w+\s+\**)+(\w+)\s*\([^)]*\)\s*;'

foreach($filePath in $args) {
    $fileContent = Get-Content $filePath -Raw
    # Find all ffi.cdef snippets
    $matches = [regex]::Matches($fileContent, $ffiSnippetPattern)
    foreach ($match in $matches) {
        $cSourceCode = $match.Groups[1].Value
        # Extract function names from the C source code
        [regex]::Matches($cSourceCode, $funcDeclarationPattern) | ForEach-Object  {
          $functionNames += $_.Groups[1].Value.Trim()
        }
    }
}
# Write module definition to stdout
"EXPORTS"
$functionNames | Sort -Unique


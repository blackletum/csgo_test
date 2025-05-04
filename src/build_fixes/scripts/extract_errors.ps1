param(
  [Parameter(Mandatory)][string]$InputFile,
  [Parameter(Mandatory)][string]$OutputFile
)

$MMOutputFile = "logs\mm_loadu_si64_issues.txt"

# Clear output files first
Clear-Content -Path $OutputFile -ErrorAction SilentlyContinue
Clear-Content -Path $MMOutputFile -ErrorAction SilentlyContinue

# Process each line
Get-Content $InputFile | ForEach-Object {

    # General MSVC error capture
    if ($_ -match '^(?<file>[A-Za-z]:\\[^()]+)\((?<line>\d+)\):\s*error\s+(?<code>C\d+):\s*(?<message>.*?)(?:\s\([^)]+\))?$') {
        Add-Content $OutputFile @(
            "File: $($matches['file'])"
            "Line: $($matches['line'])"
            "Error code: $($matches['code'])"
            "Error text: $($matches['message'])"
            ""
        )
    }

    # Specific _mm_loadu_si64 capture
    if ($_ -match "_mm_loadu_si64") {
        if ($_ -match '^.+error\s+(?<code>C\d+):\s*(?<message>.*?)\s+\(compiling source file (?<sourcefile>.+?)\)\s+\[.*$') {
            Add-Content $MMOutputFile @(
                "File: $($matches['sourcefile'])"
                "Error code: $($matches['code'])"
                "Error text: $($matches['message'])"
                ""
            )
        }
    }

}

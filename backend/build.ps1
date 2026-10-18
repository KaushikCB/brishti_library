# PowerShell build and test script for BrishtiLibrary CSE214 Backend

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "==> Compiling CSE214 Algorithm Test Suite..." -ForegroundColor Cyan
$srcFiles = @(
    (Join-Path $scriptDir "src\algorithms\search.c"),
    (Join-Path $scriptDir "src\algorithms\sort.c"),
    (Join-Path $scriptDir "src\algorithms\greedy.c"),
    (Join-Path $scriptDir "src\algorithms\knapsack.c"),
    (Join-Path $scriptDir "src\algorithms\dijkstra.c"),
    (Join-Path $scriptDir "tests\test_algorithms.c")
)
$includeDir = Join-Path $scriptDir "include"
$outputExe = Join-Path $scriptDir "tests\test_algorithms.exe"

& gcc -std=c99 -Wall -Wextra -O2 "-I$includeDir" $srcFiles -o $outputExe
if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed with exit code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "==> Running Test Suite..." -ForegroundColor Green
& $outputExe
exit $LASTEXITCODE

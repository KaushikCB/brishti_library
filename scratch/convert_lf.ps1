$paths = @(
    "frontend/css/style.css",
    "frontend/css/admin.css",
    "frontend/css/algorithms.css",
    "frontend/index.html",
    "frontend/pages/admin.html",
    "frontend/pages/algorithms.html",
    "frontend/js/app.js",
    "frontend/js/admin.js"
)
foreach ($p in $paths) {
    if (Test-Path $p) {
        $content = [System.IO.File]::ReadAllText($p)
        $content = $content.Replace("`r`n", "`n")
        [System.IO.File]::WriteAllText($p, $content, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host "Normalized $p to LF"
    }
}

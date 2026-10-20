$chromePath = "C:\Program Files\Google\Chrome\Application\chrome.exe"
$edgePath = "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
$htmlPath = (Resolve-Path "scratch\progress_report.html").Path
$pdfPath = Join-Path (Get-Location).Path "BrishtiLibray_Progress_Report.pdf"

# Try Edge first, then Chrome
$browser = if (Test-Path $edgePath) { $edgePath } else { $chromePath }
Write-Host "Using browser: $browser"
Write-Host "HTML Source: $htmlPath"
Write-Host "PDF Output: $pdfPath"

$args = @(
    "--headless",
    "--disable-gpu",
    "--no-sandbox",
    "--no-pdf-header-footer",
    "--print-to-pdf=`"$pdfPath`"",
    "`"$htmlPath`""
)

$process = Start-Process -FilePath $browser -ArgumentList $args -Wait -PassThru
Write-Host "Exit Code:" $process.ExitCode

if (Test-Path $pdfPath) {
    $info = Get-Item $pdfPath
    Write-Host "SUCCESS: Generated $pdfPath ($($info.Length) bytes)" -ForegroundColor Green
} else {
    Write-Host "Trying with Chrome --headless=new..."
    $chromeArgs = @(
        "--headless=new",
        "--disable-gpu",
        "--no-sandbox",
        "--no-pdf-header-footer",
        "--print-to-pdf=`"$pdfPath`"",
        "`"$htmlPath`""
    )
    $proc2 = Start-Process -FilePath $chromePath -ArgumentList $chromeArgs -Wait -PassThru
    if (Test-Path $pdfPath) {
        $info = Get-Item $pdfPath
        Write-Host "SUCCESS: Generated $pdfPath ($($info.Length) bytes)" -ForegroundColor Green
    } else {
        Write-Host "FAILED: PDF not found" -ForegroundColor Red
    }
}

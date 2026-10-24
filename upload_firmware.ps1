param(
    [string]$DeviceUrl = "http://192.168.31.168",
    [ValidateSet("esp32s3", "portenta_h7_m7")]
    [string]$Environment = "esp32s3"
)

$extension = if ($Environment -eq "esp32s3") { "bin" } else { "ota" }
$file = Join-Path $PSScriptRoot ".pio\build\$Environment\firmware.$extension"
$url = "$($DeviceUrl.TrimEnd('/'))/update"
if (-not (Test-Path -LiteralPath $file)) {
    Write-Error "Firmware missing. Run 'pio run -e $Environment' first."
    exit 1
}

$size   = (Get-Item $file).Length
$sizeKB = [math]::Round($size / 1024)
$name   = Split-Path $file -Leaf

Write-Host ""
Write-Host "  File : $name  ($sizeKB KB)"
Write-Host "  Target: $url"
Write-Host ""

# curl --progress-bar shows a live transfer bar in the terminal
# -w adds transfer stats after completion
$result = & curl.exe `
    --fail-with-body `
    --progress-bar `
    -X POST $url `
    -F "firmware=@$file" `
    --max-time 300 `
    -w "`n  Speed: %{speed_upload} B/s   Time: %{time_total}s" `
    2>&1

Write-Host ""
Write-Host $result
if ($LASTEXITCODE -ne 0 -or $result -match "failed|error|Invalid|Incomplete") {
    Write-Host "Upload failed; the device was not restarted." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Done. Device is rebooting..." -ForegroundColor Green

# Start API server first
Write-Host "Starting API Server..." -ForegroundColor Green
$apiProcess = Start-Process -FilePath "out\build\x64-Debug\tracker_api.exe" -PassThru

# Wait for API to initialize (adjust time as needed)
Start-Sleep -Seconds 2

# Start CLI client
Write-Host "Starting CLI Client..." -ForegroundColor Green
Start-Process -FilePath "out\build\x64-Debug\tracker_cli.exe"

Write-Host "Both applications started!" -ForegroundColor Cyan

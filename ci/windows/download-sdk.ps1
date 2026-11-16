# Download and unpack the GISInternals development SDK
param (
    [string]$SDK = "release-1944-x64"
)
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue" # speeds up Invoke-WebRequest

$SDK_ZIP = "$SDK-dev.zip"
$SDK_URL = "https://download.gisinternals.com/sdk/downloads/$SDK_ZIP"

Write-Host "Downloading $SDK_URL"
Invoke-WebRequest -Uri $SDK_URL -OutFile $SDK_ZIP

# 7-Zip is much faster than Expand-Archive
7z x $SDK_ZIP -osdk -y | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Unpacking $SDK_ZIP failed" }
Remove-Item $SDK_ZIP

if (Test-Path "sdk/$SDK-dev") {
    Get-ChildItem "sdk/$SDK-dev" | Move-Item -Destination sdk
    Remove-Item "sdk/$SDK-dev"
}
if (-not (Test-Path "sdk/$SDK/bin")) { throw "sdk/$SDK/bin not found after unpacking" }

# Aeostara
# Copyright (c) 2026 James Daley, Raven Forge Software.
# SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$NuGetExecutable,
    [string]$VerificationRecord,
    [switch]$AllowDirtyCandidate
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [IO.Path]::GetFullPath($BuildDirectory)
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $outputRoot) { throw 'OutputDirectory must be new; existing artifacts are never overwritten.' }
if (-not (Test-Path -LiteralPath $NuGetExecutable -PathType Leaf)) { throw 'An existing native NuGet executable is required.' }
$version = [IO.File]::ReadAllText((Join-Path $sourceRoot 'SDK_VERSION')).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+-rc\.\d+$') { throw 'SDK_VERSION must identify this prerelease candidate.' }
$sourceCommit = (& git -C $sourceRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $sourceCommit -notmatch '^[0-9a-f]{40}$') { throw 'Actual source commit unavailable.' }
$dirtyLines = @(& git -C $sourceRoot status --porcelain)
if ($LASTEXITCODE -ne 0) { throw 'Actual source status unavailable.' }
$dirty = $dirtyLines.Count -ne 0
if ($dirty -and -not $AllowDirtyCandidate) { throw 'Publication staging requires a clean committed checkout.' }
$verified = $false; $verificationHash = $null
if ($VerificationRecord) {
    $proof = Get-Content -LiteralPath $VerificationRecord -Raw | ConvertFrom-Json -AsHashtable
    if ($proof.sourceCommit -ne $sourceCommit -or $proof.sourceDirty -ne $false -or $proof.publicationEligible -ne $true) { throw 'Verification record does not qualify this exact clean source.' }
    if (@($proof.gates | Where-Object { $_.status -ne 'passed' }).Count -ne 0 -or @($proof.gates).Count -eq 0) { throw 'Verification record contains an unpassed gate.' }
    $verificationHash = (Get-FileHash -LiteralPath $VerificationRecord -Algorithm SHA256).Hash.ToLowerInvariant()
    $verified = $true
}
if ($dirty -and $verified) { throw 'A dirty candidate cannot be publication eligible.' }
function Write-Utf8([string]$Path, [string]$Text) { [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false)) }
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Relative([string]$Root, [string]$Path) { [IO.Path]::GetRelativePath($Root, $Path).Replace('\', '/') }
function Inventory([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object { Relative $Root $_.FullName } | ForEach-Object {
        if ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Reparse-point payload is prohibited.' }
        [ordered]@{ path = (Relative $Root $_.FullName); bytes = $_.Length; sha256 = (Hash $_.FullName) }
    })
}
function Zip-Files([string]$Root, [string]$Destination, [string]$Prefix = '') {
    $stream = [IO.File]::Open($Destination, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create, $true, [Text.UTF8Encoding]::new($false))
        try {
            foreach ($file in (Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object { Relative $Root $_.FullName })) {
                $name = $Prefix + (Relative $Root $file.FullName)
                $entry = $archive.CreateEntry($name, [IO.Compression.CompressionLevel]::Optimal)
                $entry.LastWriteTime = [DateTimeOffset]::new(2000, 1, 1, 0, 0, 0, [TimeSpan]::Zero)
                $entry.ExternalAttributes = 0
                $inputStream = [IO.File]::OpenRead($file.FullName); $entryStream = $entry.Open()
                try { $inputStream.CopyTo($entryStream) } finally { $entryStream.Dispose(); $inputStream.Dispose() }
            }
        } finally { $archive.Dispose() }
        $stream.Flush($true)
    } finally { $stream.Dispose() }
}
New-Item -ItemType Directory -Path $outputRoot | Out-Null
$workRoot = Join-Path $outputRoot 'staging'
$sdkRoot = Join-Path $workRoot 'sdk'
New-Item -ItemType Directory -Path $sdkRoot -Force | Out-Null
foreach ($configuration in @('Debug', 'Release')) {
    & cmake --install $buildRoot --config $configuration --prefix $sdkRoot
    if ($LASTEXITCODE -ne 0) { throw "Actual $configuration SDK installation failed." }
}
$cache = Get-Content -LiteralPath (Join-Path $buildRoot 'CMakeCache.txt') -Raw
if ($cache -notmatch '(?m)^AEOSTARA_DIAGNOSTIC_PROFILE:STRING=LEAN\r?$' -or $cache -notmatch '(?m)^AEOSTARA_INSTALL_SDK:BOOL=ON\r?$') { throw 'Only a lean SDK build can be staged.' }
$sdkReadme = [IO.File]::ReadAllText((Join-Path $sourceRoot 'docs\windows\SDK_GUIDE.md')).Replace('../../examples/sdk-client/README.md', "https://github.com/flynn33/Aeostara-Windows/blob/$sourceCommit/examples/sdk-client/README.md").Replace('../semantics/ADOPTION.md', "https://github.com/flynn33/Aeostara-Windows/blob/$sourceCommit/docs/semantics/ADOPTION.md")
Write-Utf8 (Join-Path $sdkRoot 'README.md') $sdkReadme
$payload = Inventory $sdkRoot
foreach ($item in $payload) {
    if ($item.path -notmatch '^(include/AeostaraCore/[^/]+\.h|lib/x64/(Debug|Release)/AeostaraCore\.lib|lib/cmake/Aeostara/[^/]+\.cmake|share/aeostara/(LICENSE|THIRD_PARTY_NOTICES\.md)|README\.md)$') { throw "Unexpected SDK payload: $($item.path)" }
}
foreach ($configuration in @('Debug', 'Release')) { if (-not (Test-Path -LiteralPath (Join-Path $sdkRoot "lib\x64\$configuration\AeostaraCore.lib"))) { throw 'Both binary configurations are required.' } }
$tuple = [ordered]@{ os='Windows'; architecture='x64'; compiler='MSVC 19.44.35223.0'; toolset='v143'; vcToolsVersion='14.44.35207'; windowsSdk='10.0.26100.0'; language='C++20'; debugRuntime='/MDd'; releaseRuntime='/MD'; exceptions='/EHsc'; rtti='/GR'; debugInformation='/Z7'; diagnosticBackend='LEAN'; systemLibraries=@('bcrypt','ole32') }
$sdkManifest = [ordered]@{ schema='aeostara.sdk-manifest/1'; packageId='Aeostara.Windows.SDK'; version=$version; sourceCommit=$sourceCommit; sourceDirty=$dirty; publicationEligible=$verified; compatibility=$tuple; semanticSupplementSha256='98539d83ad6199cecc26eb7b9fcb92993073a2e511ebf70191330cd46ba35dc7'; baseCommit='774b981ef2267a41c1b1a59497b2b0746a86f32b'; ashCommit='e123f5d7fdbb381179971f721a3292c31eb1cbc2'; payload=$payload; verificationRecordSha256=$verificationHash; signing='unsigned ordinary static SDK'; physicalTargetValidation='pending representative owner deployment' }
Write-Utf8 (Join-Path $sdkRoot 'share\aeostara\sdk-manifest.json') (($sdkManifest | ConvertTo-Json -Depth 20) + "`n")
$binaryName = "aeostara-windows-sdk-$version-msvc-v143-x64.zip"
Zip-Files $sdkRoot (Join-Path $outputRoot $binaryName) "aeostara-windows-sdk-$version/"
$nugetRoot = Join-Path $workRoot 'nuget'
New-Item -ItemType Directory -Path (Join-Path $nugetRoot 'build\native') -Force | Out-Null
foreach ($child in @('include','lib','share')) { Copy-Item -LiteralPath (Join-Path $sdkRoot $child) -Destination $nugetRoot -Recurse }
Copy-Item -LiteralPath (Join-Path $sdkRoot 'README.md') -Destination $nugetRoot
foreach ($file in @('Aeostara.Windows.SDK.props','Aeostara.Windows.SDK.targets')) { Copy-Item -LiteralPath (Join-Path $sourceRoot "sdk\native\$file") -Destination (Join-Path $nugetRoot 'build\native') }
$nuspec = @"
<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://schemas.microsoft.com/packaging/2013/05/nuspec.xsd">
  <metadata>
    <id>Aeostara.Windows.SDK</id><version>$version</version><authors>Jim Daley</authors><owners>flynn33</owners>
    <requireLicenseAcceptance>false</requireLicenseAcceptance><license type="file">share/aeostara/LICENSE</license><readme>README.md</readme>
    <projectUrl>https://github.com/flynn33/Aeostara-Windows</projectUrl>
    <repository type="git" url="https://github.com/flynn33/Aeostara-Windows" commit="$sourceCommit" />
    <description>Aeostara Windows native C++20 static SDK. Qualified x64 v143 MSVC 19.44.35223 with matched Debug /MDd or Release /MD and lean diagnostic backend.</description>
    <tags>native Windows C++20 x64 Aeostara</tags>
  </metadata>
  <files><file src="include\**\*" target="include" /><file src="lib\**\*" target="lib" /><file src="share\**\*" target="share" /><file src="build\native\*" target="build\native" /><file src="README.md" target="" /></files>
</package>
"@
$nuspecPath = Join-Path $nugetRoot 'Aeostara.Windows.SDK.nuspec'; Write-Utf8 $nuspecPath $nuspec
$nugetOutput = Join-Path $workRoot 'packed'; New-Item -ItemType Directory -Path $nugetOutput | Out-Null
& $NuGetExecutable pack $nuspecPath -BasePath $nugetRoot -OutputDirectory $nugetOutput -NoPackageAnalysis -NonInteractive
if ($LASTEXITCODE -ne 0) { throw 'Native NuGet pack failed.' }
$packageName = "Aeostara.Windows.SDK.$version.nupkg"
$unpacked = Join-Path $workRoot 'normalized-nuget'
[IO.Compression.ZipFile]::ExtractToDirectory((Join-Path $nugetOutput $packageName), $unpacked)
Zip-Files $unpacked (Join-Path $outputRoot $packageName)
$developerRoot = Join-Path $workRoot 'developer-kit'
New-Item -ItemType Directory -Path $developerRoot | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceRoot 'examples\sdk-client') -Destination (Join-Path $developerRoot 'sdk-client') -Recurse
Write-Utf8 (Join-Path $developerRoot 'README.md') $sdkReadme
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination $developerRoot
Copy-Item -LiteralPath (Join-Path $sourceRoot 'THIRD_PARTY_NOTICES.md') -Destination $developerRoot
foreach ($item in (Inventory $developerRoot)) {
    if ($item.path -notmatch '^(README\.md|LICENSE|THIRD_PARTY_NOTICES\.md|sdk-client/(main\.cpp|CMakeLists\.txt|packages\.config|AeostaraSdkClient\.vcxproj|synthetic-input\.json|README\.md))$') { throw "Unexpected developer-kit file: $($item.path)" }
}
$developerName = "aeostara-windows-sdk-developer-kit-$version.zip"
Zip-Files $developerRoot (Join-Path $outputRoot $developerName) "aeostara-windows-sdk-developer-kit-$version/"
$assets = @($binaryName, $packageName, $developerName) | ForEach-Object { $path = Join-Path $outputRoot $_; [ordered]@{ name=$_; bytes=(Get-Item -LiteralPath $path).Length; sha256=(Hash $path) } }
$releaseManifest = [ordered]@{ schema='aeostara.release-manifest/1'; version=$version; sourceCommit=$sourceCommit; sourceDirty=$dirty; tag="sdk/windows/v$version"; prerelease=$true; publicationEligible=$verified; compatibility=$tuple; semanticSupplementSha256=$sdkManifest.semanticSupplementSha256; verificationRecordSha256=$verificationHash; sdkPayload=$sdkManifest.payload; assets=$assets; archiveEntryTimestamp='2000-01-01T00:00:00Z'; stagingRepeatability='fixed entry ordering and timestamp; compare actual repeated output hashes'; compilerBitRepeatability='separate clean-build comparison required; no assumption'; signed=$false }
Write-Utf8 (Join-Path $outputRoot 'release-manifest.json') (($releaseManifest | ConvertTo-Json -Depth 20) + "`n")
$sumNames = @($binaryName, $packageName, $developerName, 'release-manifest.json') | Sort-Object
$sums = ($sumNames | ForEach-Object { "$(Hash (Join-Path $outputRoot $_))  $_" }) -join "`n"
Write-Utf8 (Join-Path $outputRoot 'SHA256SUMS') ($sums + "`n")
Write-Host "Staged $version from $sourceCommit; publicationEligible=$verified. Five release files are at $outputRoot."

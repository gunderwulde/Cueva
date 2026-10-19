param(
    [string]$EngineRoot = 'E:\Epic\UE_5.8',
    [string]$ArchiveDirectory = ''
)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
$project = Join-Path $workspace 'LaGarma_UE58_Android\LaGarma.uproject'
if (-not $ArchiveDirectory) { $ArchiveDirectory = Join-Path $workspace 'Builds\Quest3' }
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $uat)) { throw "Unreal Engine 5.8 not found: $EngineRoot" }
$buildTool = Join-Path $EngineRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe'
& $buildTool UnrealEditor Win64 Development "-Project=$project" -Module=MemoriaRuntime -Module=MemoriaAudioEditor -WaitMutex -NoHotReloadFromIDE -NoEngineChanges
if ($LASTEXITCODE -ne 0) { throw "Editor plugin build failed (exit $LASTEXITCODE)." }
# Module-only builds do not emit a plugin module manifest.
$engineManifest = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.modules') -Raw | ConvertFrom-Json
$pluginManifest = Join-Path $workspace 'LaGarma_UE58_Android\Plugins\MemoriaNative\Binaries\Win64\UnrealEditor.modules'
@{ BuildId = $engineManifest.BuildId; Modules = @{ MemoriaRuntime = 'UnrealEditor-MemoriaRuntime.dll'; MemoriaAudioEditor = 'UnrealEditor-MemoriaAudioEditor.dll' } } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $pluginManifest -Encoding UTF8
& $uat BuildCookRun "-project=$project" "-unrealexe=$editor" -noP4 -platform=Android -cookflavor=ASTC -clientconfig=Development -build -cook -stage -pak -iostore -package -archive "-archivedirectory=$ArchiveDirectory" -utf8output -unattended -nocompileeditor -nullrhi
if ($LASTEXITCODE -ne 0) { throw "Quest packaging failed (exit $LASTEXITCODE). See the AutomationTool log." }

"""Configure only the separate UE5.8 copy, retaining the validated UE4.22 source."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'LaGarma_UE58_Android'
# Bootstrap only: subsequent asset repairs and redirects must not be overwritten.
if (ROOT / 'Diagnostics' / 'UE58-native-audio-report.json').exists():
    raise SystemExit('Bootstrap already applied. Use BuildQuest3.ps1 to build the migrated project.')
descriptor = PROJECT / 'LaGarma.uproject'
data = json.loads(descriptor.read_text(encoding='utf-8-sig'))
data['EngineAssociation'] = '5.8'
data['Plugins'] = [{'Name': name, 'Enabled': True} for name in (
    'Niagara', 'OpenXR', 'OpenXRInput', 'PythonScriptPlugin', 'EditorScriptingUtilities', 'SequencerScripting', 'MemoriaNative') if name != 'OpenXRInput']
data['TargetPlatforms'] = ['Android', 'Windows']
descriptor.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')

def strip_section(text, section):
    return re.sub(r'(?ms)^\[' + re.escape(section) + r'\]\r?\n.*?(?=^\[|\Z)', '', text)

def values(text, section, entries):
    pattern = r'(?ms)^\[' + re.escape(section) + r'\]\r?\n(.*?)(?=^\[|\Z)'
    match = re.search(pattern, text)
    body = match.group(1) if match else ''
    for key, value in entries.items():
        body = re.sub(r'(?m)^' + re.escape(key) + r'=.*\r?\n?', '', body)
        body = body.rstrip() + '\n' + key + '=' + str(value) + '\n'
    replacement = '[' + section + ']\n' + body.rstrip() + '\n\n'
    return text[:match.start()] + replacement + text[match.end():] if match else text.rstrip() + '\n\n' + replacement

engine_file = PROJECT / 'Config' / 'DefaultEngine.ini'
engine = engine_file.read_text(encoding='utf-8-sig')
engine = strip_section(engine, 'InstalledDerivedDataBackendGraph')
engine = values(engine, '/Script/Engine.RendererSettings', {
    'r.TextureStreaming': 'True', 'r.ForwardShading': 'True', 'r.MobileHDR': 'False',
    'r.DynamicGlobalIlluminationMethod': '0', 'r.ReflectionMethod': '0',
    'r.Nanite.ProjectEnabled': 'False', 'r.Shadow.Virtual.Enable': '0',
    'r.AntiAliasingMethod': '3', 'r.Mobile.AntiAliasing': '3', 'r.MSAACount': '4',
    'vr.InstancedStereo': 'True', 'vr.MobileMultiView': 'True', 'r.Mobile.ShadingPath': '0',
    'r.Mobile.EnableMovableSpotlights': 'True', 'r.Mobile.EnableMovableSpotlightShadows': 'False'
})
engine = values(engine, '/Script/HardwareTargeting.HardwareTargetingSettings', {
    'TargetedHardwareClass': 'Mobile', 'AppliedTargetedHardwareClass': 'Mobile',
    'DefaultGraphicsPerformance': 'Scalable', 'AppliedDefaultGraphicsPerformance': 'Scalable'
})
engine = values(engine, '/Script/AndroidRuntimeSettings.AndroidRuntimeSettings', {
    'PackageName': 'com.overlat.memoria', 'ApplicationDisplayName': 'Memoria - Stories of La Garma',
    'MinSDKVersion': '29', 'TargetSDKVersion': '36', 'bBuildForArm64': 'True',
    'bBuildForX8664': 'False', 'bSupportsVulkan': 'True', 'bBuildForES31': 'False',
    'bSupportsVulkanSM5': 'False', 'bPackageDataInsideApk': 'True',
    'bEnableBundle': 'False', 'bUseExternalFilesDir': 'True', 'bPublicLogFiles': 'True',
    'Orientation': 'Landscape', 'bFullScreen': 'True'
})
engine = strip_section(engine, 'CoreRedirects')
redirects = {
    'AkAudioEvent': 'MemoriaAudioEvent', 'AkAudioBank': 'MemoriaAudioBank',
    'AkComponent': 'MemoriaAudioComponent', 'AkAmbientSound': 'MemoriaAmbientSound',
    'AkGameplayStatics': 'MemoriaAudioLibrary', 'AkCallbackInfo': 'MemoriaAudioCallbackInfo',
    'AkEventCallbackInfo': 'MemoriaAudioCallbackInfo',
    'MovieSceneAkTrack': 'MemoriaAudioTrack',
    'MovieSceneAkAudioEventTrack': 'MemoriaAudioEventTrack',
    'MovieSceneAkAudioEventSection': 'MemoriaAudioEventSection',
    'MovieSceneAkAudioRTPCTrack': 'MemoriaAudioRTPCTrack',
    'MovieSceneAkAudioRTPCSection': 'MemoriaAudioRTPCSection'
}
engine += '\n[CoreRedirects]\n'
for old, new in redirects.items():
    engine += f'+ClassRedirects=(OldName="/Script/AkAudio.{old}",NewName="/Script/MemoriaRuntime.{new}")\n'
engine += '+ClassRedirects=(OldName="/Script/SteamVR.SteamVRFunctionLibrary",NewName="/Script/MemoriaRuntime.MemoriaXRFunctionLibrary")\n'
engine += '+EnumRedirects=(OldName="/Script/AkAudio.EAkCallbackType",NewName="/Script/MemoriaRuntime.EAkCallbackType")\n'
engine += '+FunctionRedirects=(OldName="/Script/AkAudio.OnAkPostEventCallback__DelegateSignature",NewName="/Script/MemoriaRuntime.OnAkPostEventCallback__DelegateSignature")\n'
engine_file.write_text(engine, encoding='utf-8')

game_file = PROJECT / 'Config' / 'DefaultGame.ini'
game = game_file.read_text(encoding='utf-8-sig')
game = re.sub(r'(?ms)^\[/Script/AkAudio\.[^\]]+\]\n.*?(?=^\[|\Z)', '', game)
game = values(game, '/Script/UnrealEd.ProjectPackagingSettings', {
    'ForDistribution': 'False', 'BuildConfiguration': 'PPBC_Development',
    'IncludePrerequisites': 'False', 'InternationalizationPreset': 'All',
    'bCookAll': 'False', 'bSkipEditorContent': 'True', 'bCompressed': 'True',
    'UsePakFile': 'True', 'bUseIoStore': 'True'
})
game = re.sub(r'(?m)^\+DirectoriesToAlwaysStageAsUFS=.*WwiseAudio.*\n?', '', game)
game = re.sub(r'(?m)^\+MapsToCook=.*\n?', '', game)
game = re.sub(r'(?m)^\+DirectoriesToAlwaysCook=.*NativeAudio.*\n?', '', game)
game += '\n[/Script/UnrealEd.ProjectPackagingSettings]\n'
for name in ('Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX'):
    game += f'+MapsToCook=(FilePath="/Game/Assets/Scenes/{name}")\n'
game += '+DirectoriesToAlwaysCook=(Path="/Game/NativeAudio")\n+CulturesToStage=es\n'
game_file.write_text(game, encoding='utf-8')

inputs_file = PROJECT / 'Config' / 'DefaultInput.ini'
inputs = inputs_file.read_bytes().decode('utf-16' if inputs_file.read_bytes().startswith(b'\xff\xfe') else 'utf-8-sig')
# Generic MotionController_* key names were removed after UE4.22.
inputs = re.sub(r'(?m)^\+AxisConfig=.*MotionController_(Left|Right)_.*\n?', '', inputs)
inputs = re.sub(r'(?m)^\+ActionMappings=.*MotionController_(Left|Right)_Trigger.*\n?', '', inputs)
for hand in ('Left', 'Right'):
    line = f'+ActionMappings=(ActionName="Grab{hand}",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=OculusTouch_{hand}_Trigger_Click)'
    if line not in inputs:
        inputs += '\n' + line + '\n'
inputs_file.write_text(inputs, encoding='utf-8')

native_cpp = PROJECT / 'Plugins' / 'MemoriaNative' / 'Source' / 'MemoriaRuntime' / 'Private' / 'MemoriaAudio.cpp'
source = native_cpp.read_text(encoding='utf-8-sig')
source = source.replace('PC->PlayerCameraManager->GetCameraFadeAmount()', 'PC->PlayerCameraManager->FadeAmount')
source = source.replace('FadeColor.ToFColor(false)', 'FadeColor')
native_cpp.write_text(source, encoding='utf-8')
print('Configured UE5.8 copy:', PROJECT)


"""Restore Wwise configuration only in the backed-up UE5.8 project."""
from pathlib import Path
import copy
import json
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = root / 'LaGarma_UE58_Android'
assert (root / 'Backups/UE58_NativeAudio_1.0.2_BeforeWwise/LaGarma.uproject').is_file()
assert (project / 'Plugins/Wwise/Wwise.uplugin').is_file()

path = project / 'LaGarma_WwiseProject/LaGarma_WwiseProject.wproj'
tree = ET.parse(path)
wwise = tree.getroot().find('./ProjectInfo/Project')
platforms = wwise.find('Platforms')
if platforms.find('./Platform[@Name="Android"]') is None:
    template = ET.parse(root / 'Diagnostics/WwiseAndroidTemplate/WwiseAndroidTemplate.wproj')
    platforms.append(copy.deepcopy(template.find('.//Platforms/Platform[@Name="Android"]')))
properties = wwise.find('PropertyList')
for name, value in {
    'AutoSoundBankEnabled': 'True', 'AutoSoundBankAllEvents': 'True',
    'GenerateMultipleBanks': 'True', 'GenerateSoundBankJSON': 'True',
    'SoundBankGenerateEstimatedDuration': 'True', 'SoundBankGenerateMaxAttenuationInfo': 'True',
    'SoundBankGeneratePrintGUID': 'True', 'SoundBankGeneratePrintPath': 'True',
    'CopyLooseStreamedMedia': 'True', 'RemoveUnusedGeneratedFiles': 'False',
}.items():
    node = properties.find('./Property[@Name="' + name + '"]')
    if node is None:
        node = ET.SubElement(properties, 'Property', {'Name': name, 'Type': 'bool'})
    for child in list(node):
        node.remove(child)
    node.set('Value', value)
for name in ['SoundBankPaths', 'ExternalSourcesOutputPath', 'ExternalSourcesInputPath',
             'SoundBankPostGenerateCustomCmdLines', 'SoundBankPreGenerateCustomCmdLines',
             'SoundBankPostGenerateCustomCmdDescription', 'SoundBankPreGenerateCustomCmdDescription']:
    node = properties.find('./Property[@Name="' + name + '"]/ValueList')
    if node is not None and node.find('./Value[@Platform="Android"]') is None:
        value = ET.SubElement(node, 'Value', {'Platform': 'Android'})
        value.text = 'GeneratedSoundBanks\\Android\\' if name in ['SoundBankPaths', 'ExternalSourcesOutputPath'] else ''
ET.indent(tree, space='\t')
tree.write(path, encoding='utf-8', xml_declaration=True)

def settings(text, section, entries):
    pattern = r'(?ms)^\[' + re.escape(section) + r'\]\r?\n(.*?)(?=^\[|\Z)'
    match = re.search(pattern, text)
    body = match.group(1) if match else ''
    for key, value in entries.items():
        body = re.sub(r'(?m)^' + re.escape(key) + r'=.*\r?\n?', '', body)
        body = body.rstrip() + '\n' + key + '=' + value + '\n'
    replacement = '[' + section + ']\n' + body.rstrip() + '\n\n'
    return text[:match.start()] + replacement + text[match.end():] if match else text.rstrip() + '\n\n' + replacement

engine_path = project / 'Config/DefaultEngine.ini'
engine = engine_path.read_text(encoding='utf-8-sig')
engine = '\n'.join(line for line in engine.splitlines()
                   if not (line.startswith('+') and 'Redirects=' in line and
                           ('OldName="/Script/AkAudio' in line or 'OldName="/Script/MemoriaRuntime.MemoriaAudio' in line or
                            'OldName="/Script/MemoriaRuntime.EAkCallbackType' in line or
                            'OldName="/Script/MemoriaRuntime.OnAkPostEventCallback' in line))) + '\n'
mapping = {
    'MemoriaAudioEvent': 'AkAudioEvent', 'MemoriaAudioBank': 'AkAudioBank',
    'MemoriaAudioComponent': 'AkComponent', 'MemoriaAmbientSound': 'AkAmbientSound',
    'MemoriaAudioCallbackInfo': 'AkCallbackInfo',
    'MemoriaAudioEventTrack': 'MovieSceneAkAudioEventTrack', 'MemoriaAudioEventSection': 'MovieSceneAkAudioEventSection',
    'MemoriaAudioRTPCTrack': 'MovieSceneWwiseGameParameterTrack', 'MemoriaAudioRTPCSection': 'MovieSceneWwiseGameParameterSection',
}
redirects = ['+ClassRedirects=(OldName="/Script/MemoriaRuntime.' + old + '",NewName="/Script/AkAudio.' + new + '")'
             for old, new in mapping.items()]
redirects += [
    '+EnumRedirects=(OldName="/Script/MemoriaRuntime.EAkCallbackType",NewName="/Script/AkAudio.EAkCallbackType")',
    '+FunctionRedirects=(OldName="/Script/MemoriaRuntime.OnAkPostEventCallback__DelegateSignature",NewName="/Script/AkAudio.OnAkPostEventCallback__DelegateSignature")',
    '+PropertyRedirects=(OldName="/Script/MemoriaRuntime.MemoriaAudioRTPCSection.Channel",NewName="/Script/AkAudio.MovieSceneWwiseGameParameterSection.GameParameterChannel")',
]
for redirect in redirects:
    if redirect not in engine:
        engine = engine.replace('[CoreRedirects]\n', '[CoreRedirects]\n' + redirect + '\n', 1)
engine_path.write_text(engine, encoding='utf-8')

game_path = project / 'Config/DefaultGame.ini'
game = game_path.read_text(encoding='utf-8-sig')
game = settings(game, '/Script/AkAudio.AkSettings', {
    'WwiseProjectPath': '(FilePath="LaGarma_WwiseProject/LaGarma_WwiseProject.wproj")',
    'RootOutputPath': '(Path="../LaGarma_WwiseProject/GeneratedSoundBanks")',
    'WwiseStagingDirectory': '(Path="WwiseAudio")',
    'UnrealCultureToWwiseCulture': '(("en", "English(US)"),("en-US", "English(US)"),("es", "Spanish"),("es-ES", "Spanish"))',
    'InitBank': '/Game/Audio/InitBank.InitBank',
    'AudioRouting': 'EnableWwiseOnly',
    'bProjectMigrated': 'True', 'bAssetsMigrated': 'True', 'bSoundBanksTransfered': 'True',
})
game_path.write_text(game, encoding='utf-8')

descriptor = project / 'LaGarma.uproject'
data = json.loads(descriptor.read_text(encoding='utf-8-sig'))
if not any(p['Name'] == 'Wwise' for p in data['Plugins']):
    data['Plugins'].append({'Name': 'Wwise', 'Enabled': True})
descriptor.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
print('Wwise Android platform, metadata, language mappings and restoration redirects configured.')

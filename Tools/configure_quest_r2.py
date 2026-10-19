from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
config = root / 'LaGarma_UE58_Android/Config'

def update(text, section, entries):
    pattern = r'(?ms)^\[' + re.escape(section) + r'\]\n(.*?)(?=^\[|\Z)'
    found = re.search(pattern, text)
    body = found.group(1) if found else ''
    for key, value in entries.items():
        body = re.sub(r'(?m)^' + re.escape(key) + r'=.*\n?', '', body)
        body = body.rstrip() + '\n' + key + '=' + value + '\n'
    replacement = '[' + section + ']\n' + body.rstrip() + '\n\n'
    return text[:found.start()] + replacement + text[found.end():] if found else text.rstrip() + '\n\n' + replacement

engine = config / 'DefaultEngine.ini'
text = engine.read_text(encoding='utf-8-sig')
text = re.sub(r'(?m)^r\.DefaultFeature\.AntiAliasing=.*\n?', '', text)
text = update(text, '/Script/Engine.RendererSettings', {
    'r.AntiAliasingMethod': '3', 'r.Mobile.AntiAliasing': '3', 'r.MSAACount': '4',
})
text = update(text, '/Script/AndroidRuntimeSettings.AndroidRuntimeSettings', {
    'StoreVersion': '2', 'VersionDisplayName': '1.0.1',
})
engine.write_text(text, encoding='utf-8')

inputs = config / 'DefaultInput.ini'
text = inputs.read_text(encoding='utf-8-sig')
text = update(text, '/Script/Engine.InputSettings', {
    'DefaultPlayerInputClass': '/Script/EnhancedInput.EnhancedPlayerInput',
    'DefaultInputComponentClass': '/Script/EnhancedInput.EnhancedInputComponent',
})
text = update(text, '/Script/EnhancedInput.EnhancedInputDeveloperSettings', {
    'bEnableDefaultMappingContexts': 'True',
    '+DefaultMappingContexts': '(InputMappingContext="/Game/NativeVR/Input/IMC_Quest.IMC_Quest",Priority=0,bAddImmediately=True,bRegisterWithUserSettings=False)',
})
inputs.write_text(text, encoding='utf-8')

profiles = config / 'DefaultDeviceProfiles.ini'
text = profiles.read_text(encoding='utf-8-sig')
for section in ['Android_OpenXR DeviceProfile', 'Meta_Quest_3 DeviceProfile', 'Meta_Quest_3S DeviceProfile']:
    if '[' + section + ']' not in text:
        text += '\n[' + section + ']\n+CVars=r.AntiAliasingMethod=3\n+CVars=r.Mobile.AntiAliasing=3\n+CVars=r.MSAACount=4\n'
profiles.write_text(text, encoding='utf-8')

game = config / 'DefaultGame.ini'
text = game.read_text(encoding='utf-8-sig')
line = '+DirectoriesToAlwaysCook=(Path="/Game/NativeVR")'
if line not in text:
    text += '\n[/Script/UnrealEd.ProjectPackagingSettings]\n' + line + '\n'
game.write_text(text, encoding='utf-8')
print('Quest r2: MSAA 4x, Enhanced Input, version code 2 configured.')

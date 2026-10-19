import unreal
import os
import json

bp = unreal.load_asset('/Game/Blueprints/MotionControllerPawn')
if not unreal.MemoriaAudioEditorLibrary.repair_quest_pawn(bp):
    raise RuntimeError('Quest pawn repair failed')
if not unreal.EditorAssetLibrary.save_loaded_asset(bp, False):
    raise RuntimeError('Could not save repaired pawn')
settings = unreal.get_default_object(unreal.InputSettings)
report = {'blueprint': bp.get_path_name(), 'compiled': True,
    'player_input': str(settings.get_editor_property('default_player_input_class')),
    'input_component': str(settings.get_editor_property('default_input_component_class')),
    'assets': []}
for path in ['/Game/NativeVR/Input/IA_SelectLeft', '/Game/NativeVR/Input/IA_SelectRight', '/Game/NativeVR/Input/IMC_Quest', '/Game/NativeVR/M_SelectionRay']:
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError('Missing Quest asset: ' + path)
    report['assets'].append(asset.get_path_name())
for name in ['r.AntiAliasingMethod', 'r.Mobile.AntiAliasing', 'r.MSAACount']:
    report[name] = unreal.SystemLibrary.get_console_variable_int_value(name)
assert report['r.AntiAliasingMethod'] == 3
assert report['r.Mobile.AntiAliasing'] == 3
assert report['r.MSAACount'] == 4
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Quest-r2-repair.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_QUEST_R2_REPAIR_COMPLETE')

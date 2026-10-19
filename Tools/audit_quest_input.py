import unreal
import json
import os

report = {}
settings = unreal.get_default_object(unreal.InputSettings)
for field in ['action_mappings', 'axis_mappings', 'default_player_input_class', 'default_input_component_class']:
    try:
        report[field] = str(settings.get_editor_property(field))
    except Exception as error:
        report[field] = str(error)
for name in ['r.AntiAliasingMethod', 'r.Mobile.AntiAliasing', 'r.MSAACount', 'r.DefaultFeature.AntiAliasing']:
    report[name] = unreal.SystemLibrary.get_console_variable_int_value(name)
for path in ['/Game/Blueprints/MotionControllerPawn', '/Game/Blueprints/BP_MotionController', '/Game/Blueprints/MainController_BP']:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    obj = unreal.get_default_object(cls)
    row = {}
    for field in ['auto_possess_player', 'auto_receive_input', 'input_priority', 'block_input', 'use_controller_rotation_yaw', 'default_player_height', 'use_mouse_control', 'base_eye_height']:
        try:
            row[field] = str(obj.get_editor_property(field))
        except Exception:
            pass
    report[path] = row
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Quest-input-before.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_QUEST_INPUT_AUDIT_COMPLETE')

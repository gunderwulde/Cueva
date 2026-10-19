import unreal
import json
import os

def vec(v):
    return [v.x, v.y, v.z]

report = {}
for path in ['/Game/Blueprints/MotionControllerPawn', '/Game/VirtualRealityBP/Blueprints/DeafultControllerPawn', '/Game/VirtualRealityBP/Blueprints/LaGarmaGameMode']:
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if not cls:
        continue
    obj = unreal.get_default_object(cls)
    props = {}
    for field in ['DefaultPlayerHeight', 'useMouseControl', 'default_pawn_class']:
        try:
            props[field] = str(obj.get_editor_property(field))
        except Exception:
            pass
    report[path] = props
for name in ['Start', 'Menu', 'Zona_I']:
    world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/' + name)
    rows = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(actor, unreal.Pawn) and 'PlayerStart' not in actor.get_class().get_name():
            continue
        row = {'auto_possess_player': str(actor.get_editor_property('auto_possess_player')) if isinstance(actor, unreal.Pawn) else None, 'auto_receive_input': str(actor.get_editor_property('auto_receive_input')), 'actor': actor.get_name(), 'class': actor.get_class().get_name(), 'location': vec(actor.get_actor_location()), 'components': []}
        for component in actor.get_components_by_class(unreal.SceneComponent):
            row['components'].append({'name': component.get_name(), 'class': component.get_class().get_name(),
                'relative_location': vec(component.get_editor_property('relative_location'))})
        rows.append(row)
    report[name] = rows
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Quest-poses-before.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_QUEST_POSES_AUDIT_COMPLETE')

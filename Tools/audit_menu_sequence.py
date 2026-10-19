import unreal
import json
import os

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/Menu')
assert world, 'Menu map failed to load'
rows = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.LevelSequenceActor):
        sequence = actor.get_sequence()
        rows.append({'actor': actor.get_path_name(), 'label': actor.get_actor_label(),
                     'sequence': sequence.get_path_name() if sequence else None})
    if 'MainController' in actor.get_class().get_name():
        try:
            current = actor.get_editor_property('currentSequence')
            rows.append({'controller': actor.get_path_name(), 'currentSequence': current.get_path_name() if current else None})
        except Exception as exc:
            rows.append({'controller': actor.get_path_name(), 'error': str(exc)})
sequence = unreal.load_asset('/Game/Assets/Sequences/Menu')
assert sequence, 'Menu sequence missing'
report = {'actors': rows, 'sequence': sequence.get_path_name(),
          'tracks': [t.get_class().get_name() for t in sequence.get_tracks()],
          'bindings': [{'name': str(b.get_name()), 'tracks': [t.get_class().get_name() for t in b.get_tracks()]} for b in sequence.get_bindings()]}
path = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics', 'Menu-sequence-audit.json'))
with open(path, 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_MENU_AUDIT: ' + json.dumps(report))

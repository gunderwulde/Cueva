import unreal
import json
import os
import shutil

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..'))
project = os.path.abspath(unreal.Paths.project_dir())
backup = os.path.join(root, 'Backups', 'SequenceReferences_Before')
os.makedirs(backup, exist_ok=True)
targets = {'Menu': 'Menu', 'Valley': 'Valley', 'Zona_I': 'Zona_I',
           'Zona_IV': 'Zona_IV_Animation', 'Zona_IX': 'Zona_IX', 'Zona_Test': 'Zona_Test'}
report = []
for level, asset in targets.items():
    filename = os.path.join(project, 'Content', 'Assets', 'Scenes', level + '.umap')
    backup_file = os.path.join(backup, level + '.umap')
    if not os.path.exists(backup_file):
        shutil.copy2(filename, backup_file)
    world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/' + level)
    assert world
    actors = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
              if isinstance(a, unreal.LevelSequenceActor)]
    assert len(actors) == 1, 'Ambiguous sequence actor in ' + level
    actor = actors[0]
    sequence = unreal.load_asset('/Game/Assets/Sequences/' + asset)
    assert isinstance(sequence, unreal.LevelSequence), 'Missing sequence: ' + asset
    assert sequence.get_tracks() or sequence.get_bindings(), 'Empty sequence: ' + asset
    previous = actor.get_sequence()
    row = {'map': level, 'actor': actor.get_name(),
           'before': previous.get_path_name() if previous else None,
           'sequence': sequence.get_path_name()}
    actor.modify()
    actor.set_sequence(sequence)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Assets/Scenes/' + level)
    row['saved'] = True
    report.append(row)

# Reopen from disk in a separate load and validate all streamed actors together.
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/Start')
actors = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
          if isinstance(a, unreal.LevelSequenceActor)]
assert len(actors) == len(targets), 'Unexpected streamed sequence actor count'
for actor in actors:
    level = actor.get_path_name().split('/Scenes/')[1].split('.')[0]
    sequence = actor.get_sequence()
    expected = '/Game/Assets/Sequences/' + targets[level] + '.' + targets[level]
    assert sequence and sequence.get_path_name() == expected, 'Incorrect saved sequence in ' + level
for row in report:
    row['verified_from_start'] = True
with open(os.path.join(root, 'Diagnostics', 'Sequence-references-repair.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_SEQUENCE_REFERENCES_REPAIRED: ' + str(len(report)))

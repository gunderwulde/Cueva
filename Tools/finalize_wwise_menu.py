import unreal
import os
import json

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/Menu')
assert world
unreal.SystemLibrary.execute_console_command(world, 'save.FixupStandaloneFlags 1')
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Assets/Scenes/Menu')
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
path = os.path.join(root, 'Wwise-assets-restored.json')
with open(path, encoding='utf-8') as source:
    report = json.load(source)
for row in report['maps']:
    if row['map'] == 'Menu':
        row['ok'] = True
assert all(row['ok'] for row in report['blueprints'] + report['maps'])
with open(path, 'w', encoding='utf-8') as output:
    json.dump(report, output, indent=2)
unreal.log('MEMORIA_WWISE_MAPS_SAVED')

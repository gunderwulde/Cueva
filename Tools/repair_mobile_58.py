import unreal
import json
import os

paths = [
    '/Game/Assets/PostprocessMaterials/InsideWallMAT',
    '/Game/ParagonProps/Agora/Props/MaterialLayers/ML_MossLayer_WorldAligned',
    '/Game/ParagonProps/Monolith/Ruins/MaterialFunctions/MF_JungleAO',
    '/Game/FX/characters/Particles/Tipo2/CazadorTipo2_System1',
]
report = []
for path in paths:
    if os.environ.get('MEMORIA_REPAIR_FILTER') and not path.endswith('/' + os.environ['MEMORIA_REPAIR_FILTER']):
        continue
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError('Missing mobile repair asset: ' + path)
    changes = unreal.MemoriaAudioEditorLibrary.repair_mobile_asset(asset)
    if not changes or not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Could not repair and save: ' + path)
    report.append({'asset': path, 'repairs': changes})
    unreal.log('MEMORIA_MOBILE_REPAIRED: ' + path)
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
report_path = os.path.join(root, 'UE58-mobile-repairs.json')
if os.environ.get('MEMORIA_REPAIR_FILTER') and os.path.isfile(report_path):
    with open(report_path, encoding='utf-8-sig') as previous:
        merged = {row['asset']: row for row in json.load(previous)}
    merged.update({row['asset']: row for row in report})
    report = list(merged.values())
with open(report_path, 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_MOBILE_REPAIR_COMPLETE')

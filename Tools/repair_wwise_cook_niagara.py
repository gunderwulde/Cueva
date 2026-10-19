import unreal
import json
import os

path = '/Game/FX/characters/Particles/Tipo2/NiniaTipo2_System2'
asset = unreal.load_asset(path)
assert asset
changes = unreal.MemoriaAudioEditorLibrary.repair_mobile_asset(asset)
assert changes and unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Wwise-Niagara-repair.json'), 'w', encoding='utf-8') as output:
    json.dump({'asset': path, 'repairs': changes, 'saved': True}, output, indent=2)
unreal.log('MEMORIA_NINIA_NIAGARA_REPAIRED')

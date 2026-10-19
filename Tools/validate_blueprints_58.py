import unreal
import json
import os

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
report = []
for item in registry.get_assets_by_path('/Game', recursive=True):
    class_name = str(item.asset_class_path.asset_name)
    if not class_name.endswith('Blueprint'):
        continue
    bp = item.get_asset()
    ok = bool(bp) and unreal.MemoriaAudioEditorLibrary.compile_native_blueprint(bp)
    if ok:
        ok = unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
    report.append({'blueprint': bp.get_path_name() if bp else str(item.package_name), 'ok': ok})
with open(os.path.join(root, 'UE58-blueprint-validation.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
if any(not row['ok'] for row in report):
    raise RuntimeError('One or more Blueprints failed validation')
unreal.log('MEMORIA_ALL_NATIVE_BLUEPRINTS_OK: ' + str(len(report)))


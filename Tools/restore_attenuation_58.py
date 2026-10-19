import unreal
import os
import json
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(unreal.Paths.project_dir()).resolve()
nodes, parents = {}, {}
for file in (root / 'LaGarma_WwiseProject').rglob('*.wwu'):
    if '.cache' in file.parts:
        continue
    for node in ET.parse(file).iter():
        if node.get('ID') and node.tag not in ('ObjectRef', 'ActiveSource'):
            nodes[node.get('ID')] = node
        for child in node:
            parents[child] = node
report = []
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
for asset_data in registry.get_assets_by_path('/Game/Audio', recursive=True):
    if str(asset_data.asset_class_path.asset_name) != 'MemoriaAudioEvent':
        continue
    asset = asset_data.get_asset()
    actions = list(asset.native_actions)
    for action in actions:
        target = nodes.get(action.target_id)
        reference = None
        while target is not None:
            reference = target.find('./ReferenceList/Reference[@Name="Attenuation"]/ObjectRef')
            if reference is not None:
                break
            target = parents.get(target)
        attenuation = nodes.get(reference.get('ID')) if reference is not None else None
        curve = []
        if attenuation is not None:
            for point in attenuation.findall('./CurveUsageInfoList/VolumeDryUsage/CurveUsageInfo/Curve/PointList/Point'):
                curve.append(unreal.Vector2D(float(point.findtext('XPos')), float(point.findtext('YPos'))))
        action.distance_curve = curve
    asset.native_actions = actions
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Failed to save attenuation: ' + asset.get_path_name())
    report.append({'event': asset.get_path_name(), 'spatial_actions': sum(a.spatialized for a in actions),
                   'distance_curves': sum(bool(a.distance_curve) for a in actions)})
with open(root.parent / 'Diagnostics' / 'UE58-attenuation-report.json', 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_ATTENUATION_COMPLETE: ' + str(len(report)))

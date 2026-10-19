"""Replace a broken, texture-less tree billboard with reduced original geometry."""
import unreal
import os
import shutil

tree_path = '/Game/KiteDemo/Environments/Trees/ScotsPineTall_01/ScotsPineTall_01'
billboard_path = tree_path + '_Billboard_Mat'
tree = unreal.EditorAssetLibrary.load_asset(tree_path)
if not tree:
    raise RuntimeError('Original Scots pine mesh is missing')
if not unreal.MemoriaMigrationLibrary.remove_tree_billboard_lod(tree):
    unreal.log('MEMORIA_TREE: final LOD is already geometry; retaining all existing LODs')

leaf_material = unreal.EditorAssetLibrary.load_asset(
    '/Game/KiteDemo/Environments/Trees/ScotsPine_01/ScotsPine_01_Leaves_Mat')
for index in range(32):
    material = tree.get_material(index)
    if material and material.get_name() == 'ScotsPineTall_01_Billboard_Mat':
        tree.set_material(index, leaf_material)
if not unreal.EditorAssetLibrary.save_loaded_asset(tree, False):
    raise RuntimeError('Could not save the repaired tree')

# The unusable material and its original references remain recoverable in the
# complete original backup. Keeping it under Content would still fail CookAll.
if unreal.EditorAssetLibrary.does_asset_exist(billboard_path):
    if not unreal.EditorAssetLibrary.delete_asset(billboard_path):
        raise RuntimeError('Could not remove the now-unused invalid billboard material')
unreal.log('MEMORIA_TREE_REPAIRED: original geometry LODs retained, missing-texture billboard removed')



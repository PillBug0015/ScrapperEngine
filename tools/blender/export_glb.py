"""Export a .blend as a game-ready .glb.

    Blender -b --factory-startup Model.blend --python export_glb.py -- Model.glb

Naming conventions the engine reads back:
    COL_*      convex collider pieces (each piece must be convex)
    STATION_*  (or any empty without children) named spots for gameplay
    REF_*      scale references, not exported
"""
import sys

import bmesh
import bpy

CONVEX_TOLERANCE = 0.98  # mesh volume / hull volume below this means the piece is visibly concave


def concavity(obj):
    mesh = bmesh.new()
    mesh.from_mesh(obj.data)
    volume = abs(mesh.calc_volume())
    hull = bmesh.new()  # points only, so the result holds nothing but the hull's faces
    for vert in mesh.verts:
        hull.verts.new(vert.co)
    bmesh.ops.convex_hull(hull, input=hull.verts[:])
    hull_volume = abs(hull.calc_volume())
    mesh.free()
    hull.free()
    return 1.0 if hull_volume == 0.0 else volume / hull_volume


def main():
    output = sys.argv[sys.argv.index("--") + 1]

    exported = [o for o in bpy.data.objects if not o.name.startswith("REF_")]
    for obj in bpy.data.objects:
        obj.select_set(obj in exported)

    for obj in exported:
        if obj.name.startswith("COL_") and obj.type == 'MESH':
            ratio = concavity(obj)
            if ratio < CONVEX_TOLERANCE:
                print(f"[export] WARNING {obj.name} is concave ({ratio:.0%} of its hull); "
                      f"physics will fill it in. Split it into convex pieces.")

    bpy.ops.export_scene.gltf(
        filepath=output,
        export_format='GLB',
        use_selection=True,
        export_apply=True,   # modifiers applied
        export_yup=True,     # Blender Z-up -> engine Y-up (Blender +Y becomes the engine's forward -Z)
        export_cameras=False,
        export_lights=False,
    )

    colliders = sum(1 for o in exported if o.name.startswith("COL_"))
    meshes = sum(1 for o in exported if o.type == 'MESH' and not o.name.startswith("COL_"))
    markers = sum(1 for o in exported if o.type == 'EMPTY' and not o.children)
    print(f"[export] {output}: {meshes} meshes, {colliders} colliders, {markers} markers")


main()

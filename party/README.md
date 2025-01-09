Conventions
* Right hand coordinate system
* positive rotation is right hand thumb along vector and direction hands curl
* column vectors (i.e. basis vectors are columns when written down on page)
* forward, right, up is (-z, +x, +y)


Engine Features Desired for Gameplay Prototyping
* Directional Lighting (i.e. the sun)
    * (Optional bonus) basic shadows
* Basic Text Rendering
* Basic Model Loading (Possibly not even necessary as I can try to use procedural geometry for as long as possible)


TODO
* Bugs
    * segfault when switching scenes with icosphere with too many subdivisions. sometimes segfaults on program
    startup as well.
* Collisions
    * Mat4Point
        * handle up x z == 0
    * implement time of impact calculation
    * floating point robustness
    * calculate contact points
        * in collision query (like face face/edge edge) return id[2] of features (either two faces) or (two edges). calculate most antiparallel face
        * then do a search with the face id (which is a triangle). if surrounding triangles from the verts
        point in same normal, then also use those verts to clip.
* Rendering
    * get lights + lighting working again.
* Core
    * impl loading blender scene
    * impl hot reloading
    * impl audio and sound playback.
    * stop using raylib for input
    * stop using raylib for window creation


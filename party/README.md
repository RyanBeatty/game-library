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
* Collisions
    * Mat4Point
        * handle up x z == 0
    * Create basic physics demo where cube is bouncing around room (reflect outgoing velocity around collision normal). collision response.
    * calculate contact points
        * in collision query (like face face/edge edge) return id[2] of features (either two faces) or (two edges). calculate most antiparallel face
        * then do a search with the face id (which is a triangle). if surrounding triangles from the verts
        point in same normal, then also use those verts to clip.
* Rendering
    * switch to DrawCommands
    * support being able to shade meshes one solid color. useful for controlling debug info.
    * get lights + lighting working again.


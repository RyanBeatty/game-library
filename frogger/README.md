Conventions
* Right hand coordinate system
* positive rotation is right hand thumb along vector and direction hands curl
* column vectors (i.e. basis vectors are columns when written down on page)
* forward, right, up is (-z, +x, +y)
* 2d/screenspace drawing maps (0, 0) to bottom left corner. +y goes up


Engine Features Desired for Gameplay Prototyping
* Directional Lighting (i.e. the sun)
    * (Optional bonus) basic shadows
* Basic Text Rendering
* Basic Model Loading (Possibly not even necessary as I can try to use procedural geometry for as long as possible)


TODO FROGGER
* feature
    * Tune collision areas.
    * sounds
    * exhaust smoke particle effects on the cars
* bug
    * Have a visual glitch. slight texture bleeding on Car b (blue car) where left side of red truck bleeds in on some frames. Possibly need to consider doing a half texel offset, but that causes some jiterring when moving at low speeds. Maybe look up pixel perfect rendering? or fat pixel shaders?
    * (ADDRESSED WITH POTENTIAL SOLUTION FOR NOW) draw order issue. once frog clears a goal, it will appear under logs after respawn. I should implement z sorting. But when to know how to do this in the renderer?

TODO
* Bugs
    * segfault when switching scenes with icosphere with too many subdivisions. sometimes segfaults on program
    startup as well.
* Collisions
    * Mat4Point
        * handle up x z == 0
    * Cleanup collision handling:
        * I need to change the actual positiona and velocity updates to be constraint based. Example problem:
        If I don't run movement + collision with substeps, then the ball can hit two blocks and fly off in a weird direction. To avoid, I can still have custom resolution code for when to play sounds or increase score, but the actual position and velocity updates should be a constraint solver based. Hitting two blocks would then cause 4 constraints (2 pos + 2 vel) to be created and those can be solved together. Instead of solving them
        indepdenatly like I am (and not even checking if the constraints are still violated when resolving collision)

    * OLD POTENTIALLY:
    * implement time of impact calculation
    * floating point robustness
    * calculate contact points
        * in collision query (like face face/edge edge) return id[2] of features (either two faces) or (two edges). calculate most antiparallel face
        * then do a search with the face id (which is a triangle). if surrounding triangles from the verts
        point in same normal, then also use those verts to clip.
* Rendering
    * get lights + lighting working again.
* Core
    * impl particle system
    * impl loading blender scene
    * impl hot reloading
    * stop using raylib for input
    * stop using raylib for window creation


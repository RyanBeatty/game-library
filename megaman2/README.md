Conventions
* Right hand coordinate system
* positive rotation is right hand thumb along vector and direction hands curl
* column vectors (i.e. basis vectors are columns when written down on page)
* forward, right, up is (-z, +x, +y)
* 2d/screenspace drawing maps (0, 0) to bottom left corner. +y goes up


Engine Improvments/Thoughts
* The way shader passes work with multiple shaders is kind of clunky. Especially the render to texture and then to sreen bit. Seems like I don't have great support for switching framebuffers between shader passes. I'm not sure what the right structure here is at this time tbh.
* At some point we need to tackle memory allocation.
* Stop using raylib for input handling
* Stop using raylib for window + opengl context creation.
* The way graphics info is set is a bit weird. Really I should be able only have to initialize data from an aserect or aserpite animation once and then do some simple pointer swap of graphicsinfo state between these intialized things. Might need to take into account animation state. Maybe that should be moved though?
    - Having fame_dt be inside the textureinfo doesn't make much sense. This should probably be on the GraphicsInfo itself

MEGAMAN GOALS
* at least 1 stage + boss fight
* ideally 1 new stage + boss fight
* colors
    * choose a color pallete to import into aseprite. ONLY USE THESE COLORS
    * 3 color character sprites
* sound
    * make a stage track that loops


TODO MEGAMAN
* Issues with ladder dismount. from woodman room 0 to room 1. hard to dismount the ladder.
* Collision system issue:
    * when walking over two ground bboxes. player character will start bouncing rapidly. this is probably due to the fact that we are resolving collision constraints twice (one for each ground plane) but the second resolution probably isn't necessary since the resolution of the first constraint will also satisfy the second. This might be fixed by moving to iterative solver approach where we check each constraint for violation before attempting to resolve.
* I should seriously consider integrating with something like ldtk. Making new stages is painful and error prone. For now though lets keep managing stages via code. I'm still in prototyping phase of just getting some mechanics into the game.
* Memory
    * unittest alignment
    * Allocate global memory
    * Dynamic array
    * Convert collision infos to be list of colliders
* scenes
    * boss scene


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


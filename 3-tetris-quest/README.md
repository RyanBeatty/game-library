TARGET DEV TIME BUDGET: 60 HOURS
CURRENT DEV TIME: 24.0 HOURS

GAME
* Goal
    * Sort laundry into correct bins to get points. Get the highest score in 2 minutes. Avoid bombs
* 3 bins to sort - shirts + tops, pants + skirts, socks + underwear. each maps to directional key
* correct sorting scores points
* successive correct scoring builds combo
* bombs appear occasionally, must remove. if sorted, breaks combo (and potentially stuns 1s or loses health)
* rotate cothing sprites to make unclear which bin they need to be sorted into?
* 10 different pairs of clothes
* color palette: https://lospec.com/palette-list/pacific

TAKEAWAYS
* Derived positions are nice and fast to start out with but are a PITA when you want to do animations. Everything should store a position (game + visual pos).




Conventions
* Right hand coordinate system
* positive rotation is right hand thumb along vector and direction hands curl
* column vectors (i.e. basis vectors are columns when written down on page)
* forward, right, up is (-z, +x, +y)
* 2d/screenspace drawing maps (0, 0) to bottom left corner. +y goes up


Refactors
* Renderer
    * Goals, make it more like sokol. EVERYTHING GETS TRANSLATED TO A DRAW COMMAND IN A SINGLE BUFFER NO BUFFER PER PASS
    * Next steps:
        * Start to work on the upscaling thing
            * Consider making a ShaderPassSpec 
            * Make Begin Pass reference ShaderPass struct now?
    * Stretch goal: move to using UBOs for the uniform bindings as it looks wayyyyy more scalable than my enable and uniform setting functions.


Engine Improvments/Thoughts
* On asset loading (e.g. sprites, animations, etc). It feels like I want 2 style of functions
    * one that deals with immediate loading of "raw" assets (e.g. LoadTextureFromFile). these will be used to actually load data from disk and upload to gpu
    * higher level pipelined loading functions. These will operate on higher level constructs that will be a bit agnostic to how the underlying gpu data is stored. e.g. like LoadSprite, LoadAnimation. The user will provide these calls with options for loading (like which filter to use for textures, potentially spritesheet grouping affinity). The user will specifiy a PipelineRegion begin and end. Within the pipeline, it will make Loading calls which will not load and return immediately. Instead after the pipelineend, we will go and fetch all requested assets, load them, try to optimally store on GPU and then allow the user to query back the loaded details
    * so first functions for immediate loading (and debuggging), second set for convenience and iteration.
* The way shader passes work with multiple shaders is kind of clunky. Especially the render to texture and then to sreen bit. Seems like I don't have great support for switching framebuffers between shader passes. I'm not sure what the right structure here is at this time tbh.
* The way graphics info is set is a bit weird. Really I should be able only have to initialize data from an aserect or aserpite animation once and then do some simple pointer swap of graphicsinfo state between these intialized things. Might need to take into account animation state. Maybe that should be moved though?
    - Having fame_dt be inside the textureinfo doesn't make much sense. This should probably be on the GraphicsInfo itself
* There is still aliasing issues with normal geometry when it is rotated or scaled. (This is separate from the sub pixel rendering issues I'm seeing). I believe MSAA will fix this (https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing). Probably would be a good opportunity to tackle better abstraction for frame buffer related operations.
* If the first scene rendered is the stage scene, we fall through the world because the first value of dt is so large that our physics teleports us downward. I think this should be addressed by fixing the timestep and decoupling the renderer from update logic.
* Issues on win32 where sprites aren't rendering during stage scene. I think this might be due to near/far clip planes being set to [-1, 1]. Not sure why this is a problem on win32 and not raylib.


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


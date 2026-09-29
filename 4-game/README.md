TARGET DEV TIME BUDGET: 24 hours
CURRENT DEV TIME BUDGET: 6.5 hrs


TAKEAWAYS FOR NEXT GAME
* WE SHOULD ONLY HAVE ONE RENDER PASS AND THEN ALSO REMOVE THE UPSCALING DRAW CALL. It should work like so:
    * author asset sizes and positions at some fixed resolution (idk say like 640 x 320)
    * render to an offscreen texture that is the same size as the final resolution (say 1920 x 1080)
    * only use the logical coordinates the (640 x 320 coordinates) for drawing
    * set my single pass projection matrix bounds to match the screen. This in effects will "strectch/scale" my draw calls. We don't have quantization bugs this way as opposed to render to
    lower resolution buffer and then upscale that since the rasterizer/fragment shader will run for each pixel that goes into a logical unit
    * at this point, we can do all rendering (game, ui, text) in the same pass. For text specifically, we will need to add a "division" scale factor to dimensions to counteract
    the upscaling in order for text to look crisp since upscaled text looks like shit. So essentially if we are upscaling 3x from our logical coordinates to screen coordinates, 
    we need to divide the scale sizing of the text quads by 3 (and also play with the spacing between them) to make the text crisp. this might mean we also need to render at a higher fontsize potentially.
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


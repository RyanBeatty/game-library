// Here lies discarded code. RIP. Yes I'm well aware that I could recover this stuff via version
// control, but this is much more convenient for searching.

// static void SceneTickTennis(GameState* gstate) {
//     static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
//     static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

//     static Vec3 GRAVITY = Vec3Lit(0.0f, -9.81f, 0.0f);

//     static EntityId playerid;

//     static EntityId BALLS[MAX_ENTITY_BATCH] = {0};
//     static u32 BALLS_SIZE = 0;

//     static EntityId WALLS[MAX_ENTITY_BATCH] = {0};
//     static u32 WALLS_SIZE = 0;

//     f32 dt = gstate->dt;

//     // Init logic.
//     if (!gstate->init) {
//         viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
//         vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

//         memset(WALLS, 0, sizeof(WALLS));
//         WALLS_SIZE = 0;
//         memset(BALLS, 0, sizeof(BALLS));
//         BALLS_SIZE = 0;

//         vieworient.yaw = Deg2Rad(90.0f);

//         // Player init.
//         {
//             Entity* entity = AllocateEntity();
//             entity->pos = Vec3Lit(0.0f, 10.0f, 0.0f);
//             entity->orient = Mat4RotateY(Deg2Rad(90.0f));
//             entity->scale = Vec3Lit(15.0f, 15.0f, 15.0f);
//             // entity->mesh = GetMesh(M_ICOSPHERE);
//             // entity->color = LCLEAR;
//             FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);
//             // FlagOn(entity->flags, EFLAG_COLLIDER);

//             playerid = entity->id;
//         }

//         // BALLS init
//         {
//             EntityId* entities = BALLS;
//             u32 size = 0;

//             const Vec3 origin = Vec3Lit(-50.0f, 10.0f, 0.0f);

//             {
//                 const Vec3 positions[] = {origin};
//                 const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
//                 const f32 rotations[] = {Deg2Rad(0.0f)};
//                 Vec4 color = LRED;
//                 color.a = 0.16f;
//                 const Vec4 colors[] = {color};

//                 for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
//                     Entity* entity = AllocateEntity();
//                     entity->pos = positions[i];
//                     entity->mesh = GetMesh(M_ICOSPHERE);
//                     entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
//                     entity->color = colors[i];
//                     entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);
//                     entity->v = velocities[i];
//                     entity->phtype = PH_SPHERE;
//                     entity->last_hit = -1;
//                     FlagOn(entity->flags, EFLAG_BALL);
//                     FlagOn(entity->flags, EFLAG_COLLIDER);
//                     entities[++size] = entity->id;
//                     Assert(size < MAX_ENTITY_BATCH);
//                 }
//             }

//             BALLS_SIZE = size;
//             Assert(BALLS_SIZE < ArraySizeOf(BALLS));
//         }

//         // walls init.
//         {
//             u32 width = 20.0f;
//             u32 height = 30.0f;
//             Vec3 positions[] = {Vec3Lit(0.0f, 0.0f, 0.0f), Vec3Lit(0.0f, height, 0.0f),
//                                 Vec3Lit(0.0f, 0.0f, -0.5f * width), Vec3Lit(0.0f, 0.0f, 0.5f * width),
//                                 Vec3Lit(-50.0f, 0.0f, 0.0f)};
//             f32 rotations[] = {Deg2Rad(-90.0f), Deg2Rad(90.0f), Deg2Rad(0.0f), Deg2Rad(180.0f), Deg2Rad(90.0f)};
//             Vec3 axis[] = {Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),
//                            Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f)};
//             Vec4 colors[] = {LORANGE, LBLUE, LGRAY, LGRAY, LGRAY};

//             EntityId* entities = WALLS;
//             u32 size = WALLS_SIZE;
//             for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
//                 Entity* entity = AllocateEntity();
//                 entity->pos = positions[i];
//                 entity->mesh = GetMesh(M_QUAD);
//                 entity->orient = Mat4Rotate(rotations[i], axis[i]);
//                 entity->color = colors[i];
//                 entity->scale = Vec3Lit(10000.0f, 10000.0f, 0.0f);
//                 entity->phtype = PH_AABB;
//                 entity->friction = 0.7f;
//                 entity->restitution = 0.8f;
//                 FlagOn(entity->flags, EFLAG_COLLIDER);
//                 FlagOn(entity->flags, EFLAG_WALL);
//                 entities[++size] = entity->id;
//                 Assert(size < MAX_ENTITY_BATCH);
//             }

//             WALLS_SIZE = size;
//             Assert(WALLS_SIZE < ArraySizeOf(WALLS));
//         }
//     }

//     // Update logic.
//     {
//         Entity* player = GetEntity(playerid);

//         // Camera movement.
//         {
//             static const u8 GAMEPAD_ID = 0;
//             static bool FREECAM = false;

//             if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
//                 FREECAM = !FREECAM;
//             }

//             // Left stick movement.
//             {
//                 static const f32 WALK_VEL = 20.0f;
//                 static const f32 RUN_VEL = 2.0f * WALK_VEL;

//                 f32 vel = WALK_VEL;
//                 if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
//                     vel = RUN_VEL;
//                 }

//                 f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
//                 f32 y = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y);
//                 Vec3 dir_model = Vec3Lit(x, 0.0f, y);

//                 Vec3 bodyorient = vieworient;
//                 // If not freecam, then limiting pitch to 0 will do fps style movement along current plane.
//                 bodyorient.pitch = FREECAM ? bodyorient.pitch : 0.0f;
//                 Mat4 rotation = Mat4RotateEuler(bodyorient);

//                 Vec3 dir_world = Vec3Transform(dir_model, rotation);
//                 Vec3 mov_world = Vec3Scale(dir_world, vel * dt);
//                 player->pos = Vec3Add(player->pos, mov_world);
//             }

//             // Right stick movement.
//             {
//                 f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
//                 f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
//                 if (x != 0.0f || y != 0.0f) {
//                     static const rad VEL = Deg2Rad(90.0f);

//                     // rotate orient.
//                     vieworient.yaw += -1.0f * x * VEL * dt;
//                     vieworient.pitch += y * VEL * dt;
//                     // Clamp pitch to avoid looking too high/low which would sign flip things.
//                     vieworient.pitch = f32clamp(vieworient.pitch, Deg2Rad(-89.0f), Deg2Rad(89.0f));
//                     player->orient = Mat4RotateEuler(vieworient);
//                 }
//             }
//         }

//         // Scene4 update.
//         {
//             {
//                 EntityManager* em = GetEntityManager();
//                 u32 esize = ArraySizeOf(em->entities);
//                 for (u32 i = 0; i < esize; ++i) {
//                     Entity* e = GetEntity(i);
//                     if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
//                         continue;
//                     }

//                     if (TestFlag(e->flags, EFLAG_PHYSICS_ON)) {
//                         e->v = Vec3Add(e->v, Vec3Scale(GRAVITY, dt));
//                         e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
//                     }

//                     if (TestFlag(e->flags, EFLAG_BALL)) {
//                         e->v = Vec3Add(e->v, Vec3Scale(GRAVITY, dt));
//                         e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
//                     }

//                     if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
//                         {
//                             static Entity* BUMPER = NULL;
//                             if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_TRIGGER_2)) {
//                                 if (BUMPER == NULL) {
//                                     Entity* enew = AllocateEntity();
//                                     enew->mesh = GetMesh(M_CUBE);
//                                     enew->scale = Vec3Lit(1000.0f, 2.0f, 1000.0f);
//                                     enew->pos = ORIGIN;
//                                     enew->phtype = PH_AABB;
//                                     enew->friction = 1.0f;
//                                     enew->restitution = 1.2f;
//                                     FlagOn(enew->flags, EFLAG_COLLIDER);
//                                     FlagOn(enew->flags, EFLAG_BUMPER);
//                                     BUMPER = enew;
//                                 }
//                             } else if (BUMPER != NULL) {
//                                 FlagOff(BUMPER->flags, EFLAG_ACTIVE);
//                                 BUMPER = NULL;
//                             }
//                         }

//                         {
//                             static Entity* BUMPER = NULL;
//                             if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
//                                 if (BUMPER == NULL) {
//                                     Entity* enew = AllocateEntity();
//                                     enew->mesh = GetMesh(M_CUBE);
//                                     enew->scale = Vec3Lit(10.0f, 20.0f, 10.0f);
//                                     enew->phtype = PH_AABB;
//                                     enew->friction = 1.0f;
//                                     enew->restitution = 1.2f;
//                                     enew->color = LRED;
//                                     FlagOn(enew->flags, EFLAG_COLLIDER);
//                                     FlagOn(enew->flags, EFLAG_BUMPER);
//                                     FlagOff(enew->flags, EFLAG_DRAW);
//                                     BUMPER = enew;
//                                 }

//                                 Vec3 bodyorient = vieworient;
//                                 bodyorient.pitch = 0.0f;
//                                 Mat4 m = Mat4RotateEuler(bodyorient);
//                                 m = Mat4Multiply(Mat4TranslateV(e->pos), m);
//                                 BUMPER->pos = Vec3WTransform(Vec3Scale(FORWARD, 10.0f), m);
//                             } else if (BUMPER != NULL) {
//                                 FlagOff(BUMPER->flags, EFLAG_ACTIVE);
//                                 BUMPER = NULL;
//                             }
//                         }
//                     }
//                 }

//                 // Collisions.
//                 {
//                     CollisionInfo collisions[10] = {0};
//                     u32 csize = 0;

//                     // Detection.
//                     u32 size = ArraySizeOf(em->entities);
//                     for (u32 i = 0; i < size; ++i) {
//                         Entity* e0 = GetEntity(i);
//                         if (!TestFlag(e0->flags, EFLAG_ACTIVE) || !TestFlag(e0->flags, EFLAG_COLLIDER)) {
//                             continue;
//                         }

//                         Polyhedron p0 = EntityBoundingVolume(e0);
//                         for (u32 j = i + 1; j < size; ++j) {
//                             Entity* e1 = GetEntity(j);
//                             if (!TestFlag(e1->flags, EFLAG_ACTIVE) || !TestFlag(e1->flags, EFLAG_COLLIDER)) {
//                                 continue;
//                             }

//                             Polyhedron p1 = EntityBoundingVolume(e1);

//                             CollisionInfo collision = {0};
//                             bool collides = TestPolyhedronVsPolyhedron(&p0, &p1, &collision.normal,
//                             &collision.depth); if (collides) {
//                                 collision.id0 = e0->id;
//                                 collision.id1 = e1->id;
//                                 collisions[csize++] = collision;
//                                 Assert(csize < 10);
//                             }
//                         }
//                     }

//                     // Response.
//                     for (u32 i = 0; i < csize; ++i) {
//                         CollisionInfo* collision = &collisions[i];
//                         Entity* e0 = GetEntity(collision->id0);
//                         Entity* e1 = GetEntity(collision->id1);
//                         {
//                             bool e0ball = TestFlag(e0->flags, EFLAG_BALL);
//                             bool e0wall = TestFlag(e0->flags, EFLAG_WALL);
//                             bool e1ball = TestFlag(e1->flags, EFLAG_BALL);
//                             bool e1wall = TestFlag(e1->flags, EFLAG_WALL);
//                             if ((e0ball && e1wall) || (e0wall && e1ball)) {
//                                 Vec3 normal = collision->normal;
//                                 f32 depth = collision->depth;
//                                 Entity* eball = e0;
//                                 Entity* ewall = e1;
//                                 if (e0wall && e1ball) {
//                                     eball = e1;
//                                     ewall = e0;
//                                     normal = Vec3Negate(normal);
//                                 }

//                                 eball->pos = Vec3Add(eball->pos, Vec3Scale(normal, -depth));
//                                 // Reflect velocity about the contact normal.
//                                 // Remember that to reflect a vec about a normal you add the portion fo the vec in
//                                 the
//                                 // per to the negative of the portion in the parallel.
//                                 // NOTE: That parallel and perp are in relation to the colission *normal* rather than
//                                 // the surface. some texts treat it the other way around.
//                                 Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(eball->v, normal));
//                                 Vec3 v_perp = Vec3Sub(eball->v, v_parallel);
//                                 eball->v = Vec3Add(Vec3Scale(v_perp, 1.0f), Vec3Scale(v_parallel, -1.0f));

//                                 eball->last_hit = ewall->id;
//                             }
//                         }

//                         {
//                             bool e0ball = TestFlag(e0->flags, EFLAG_BALL);
//                             bool e0bumper = TestFlag(e0->flags, EFLAG_BUMPER);
//                             bool e1ball = TestFlag(e1->flags, EFLAG_BALL);
//                             bool e1bumper = TestFlag(e1->flags, EFLAG_BUMPER);
//                             if ((e0ball && e1bumper) || (e0bumper && e1ball)) {
//                                 static const f32 RETURN_SPEED = 30.0f;
//                                 static const f32 RETURN_ANGLE_RAD = Deg2Rad(10.0f);

//                                 Vec3 normal = collision->normal;
//                                 Entity* eball = e0;
//                                 Entity* ebumper = e1;
//                                 if (e0bumper && e1ball) {
//                                     eball = e1;
//                                     ebumper = e0;
//                                     normal = Vec3Negate(normal);
//                                 }

//                                 if (eball->last_hit != ebumper->id) {
//                                     Vec3 bodyorient = vieworient;
//                                     bodyorient.pitch = RETURN_ANGLE_RAD;
//                                     Mat4 rotate = Mat4RotateEuler(bodyorient);
//                                     normal = Vec3Normalize(Vec3Transform(FORWARD, rotate));
//                                     eball->v = Vec3Scale(normal, RETURN_SPEED);
//                                 }

//                                 eball->last_hit = ebumper->id;
//                             }
//                         }

//                         {
//                             bool e0physics = TestFlag(e0->flags, EFLAG_PHYSICS_ON);
//                             bool e0wall = TestFlag(e0->flags, EFLAG_WALL);
//                             bool e1physics = TestFlag(e1->flags, EFLAG_PHYSICS_ON);
//                             bool e1wall = TestFlag(e1->flags, EFLAG_WALL);
//                             if ((e0physics && e1wall) || (e0wall && e1physics)) {
//                                 Vec3 normal = collision->normal;
//                                 f32 depth = collision->depth;
//                                 Entity* ephysics = e0;
//                                 Entity* ewall = e1;
//                                 if (e0wall && e1physics) {
//                                     ephysics = e1;
//                                     ewall = e0;
//                                     normal = Vec3Negate(normal);
//                                 }

//                                 ephysics->pos = Vec3Add(ephysics->pos, Vec3Scale(normal, -depth));
//                                 // Reflect velocity about the contact normal.
//                                 // Remember that to reflect a vec about a normal you add the portion fo the vec in
//                                 the
//                                 // per to the negative of the portion in the parallel.
//                                 // NOTE: That parallel and perp are in relation to the colission *normal* rather than
//                                 // the surface. some texts treat it the other way around.
//                                 Vec3 v_parallel = Vec3Scale(normal, Vec3Dot(ephysics->v, normal));
//                                 Vec3 v_perp = Vec3Sub(ephysics->v, v_parallel);
//                                 ephysics->v = Vec3Add(Vec3Scale(v_perp, ewall->friction),
//                                                       Vec3Scale(v_parallel, -1.0f * ewall->restitution));
//                             }
//                         }
//                     }
//                 }

//                 // Draw.
//                 for (u32 i = 0; i < ArraySizeOf(em->entities); ++i) {
//                     Entity* e = GetEntity(i);
//                     if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
//                         continue;
//                     }

//                     if (TestFlag(e->flags, EFLAG_DRAW)) {
//                         Vec4 color = e->color;
//                         if (TestFlag(e->flags, EFLAG_MOB) && e->hit) {
//                             static const f32 FREQ = 2.5f;
//                             f32 t = 0.5f * (1.0f + f32sin(2.0f * LPI * FREQ * e->hit_timer));
//                             color.r = f32lerp(color.r, LRED.r, t);
//                             color.g = f32lerp(color.g, LRED.g, t);
//                             color.b = f32lerp(color.b, LRED.b, t);
//                             color.a = f32lerp(color.a, LRED.a, t);
//                         }

//                         Mat4 scale = Mat4ScaleV(e->scale);
//                         Mat4 translate = Mat4TranslateV(e->pos);
//                         Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
//                         AddCmdOld(e->mesh, model, color);
//                         DrawCmd* cmd2 = AddCmdOld(e->mesh, model, color);
//                         cmd2->wireframe = true;
//                     }

//                     if (gstate->debug && TestFlag(e->flags, EFLAG_COLLIDER)) {
//                         Sphere sphere = EntityBoundingSphere(e);
//                         DrawSphereWf(sphere.c, sphere.r, LYELLOW);

//                         AABB aabb = EntityAABB(e);
//                         DrawAABBWf(aabb.c, aabb.r, LBLUE);
//                     }
//                 }
//             }
//         }

//         // Update camera pos at the end of all this to make sure player pos is final position for frame.
//         viewpos = player->pos;
//         if (gstate->debug) {
//             // TraceLog(LOG_INFO, "pos: %f %f %f", player->pos.x, player->pos.y, player->pos.z);
//         }
//     }

//     // Drawing
//     {
//         // Begin.
//         {
//             BeginDrawing();
//             glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
//             glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//             glBindVertexArray(VAO);
//         }

//         Mat4 view = GetCameraMat(viewpos, vieworient);
//         Mat4 proj = Mat4Perspective(Deg2Rad(45.0f), (f32)GetScreenWidth() / (f32)GetScreenHeight(), 0.1f, 1000.0f);

//         // Enable/Disable culling.
//         // TODO: Make this nicer at some point. Useful for debugging.
//         {
//             if (gstate->debug) {
//                 glDisable(GL_CULL_FACE);
//             } else {
//                 glEnable(GL_CULL_FACE);
//                 glCullFace(GL_BACK);
//                 glFrontFace(GL_CCW);
//             }
//         }

//         // Use shader + setup norm + view + proj matrix.
//         LShader shader = GetShaderOld();
//         {
//             glUseProgram(shader.pid);
//             glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
//             glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
//         }

//         for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
//             DrawCmd* cmd = &CBUFFER[i];
//             glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
//             glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
//             if (cmd->wireframe) {
//                 glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//             }
//             glDrawElements(GL_TRIANGLES, cmd->meshid.ntriangles * 3, GL_UNSIGNED_INT,
//                            (void*)(cmd->meshid.ibufferidx * sizeof(GLuint)));
//             if (cmd->wireframe) {
//                 glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//             }
//         }

//         EndDrawing();
//         CmdBufferZero();  // Maybe consider moving this to be at the top of update func.
//     }
// }

// static void SceneTickItFollows(GameState* gstate) {
//     static Vec3 viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);
//     static Vec3 vieworient = Vec3Lit(0.0f, 0.0f, 0.0f);

//     static EntityId playerid;

//     static EntityId MOBS[MAX_ENTITY_BATCH] = {0};
//     static u32 MOBS_SIZE = 0;

//     static EntityId TARGETS[MAX_ENTITY_BATCH] = {0};
//     static u32 TARGETS_SIZE = 0;

//     static EntityId WALLS[MAX_ENTITY_BATCH] = {0};
//     static u32 WALLS_SIZE = 0;

//     f32 dt = gstate->dt;

//     // Init logic.
//     if (!gstate->init) {
//         viewpos = Vec3Lit(0.0f, 0.0f, 0.0f);

//         memset(MOBS, 0, sizeof(MOBS));
//         MOBS_SIZE = 0;
//         memset(WALLS, 0, sizeof(WALLS));
//         WALLS_SIZE = 0;
//         memset(TARGETS, 0, sizeof(TARGETS));
//         TARGETS_SIZE = 0;

//         vieworient.yaw = Deg2Rad(90.0f);

//         // Player init.
//         {
//             Entity* entity = AllocateEntity();
//             entity->pos = Vec3Lit(0.0f, 0.0f, 0.0f);
//             entity->orient = Mat4RotateY(Deg2Rad(90.0f));
//             FlagOn(entity->flags, EFLAG_PLAYER_CONTROLLED);

//             playerid = entity->id;
//         }

//         // MOBS init
//         {
//             EntityId* entities = MOBS;
//             u32 size = 0;

//             const Vec3 origin = Vec3Lit(-50.0f, 0.0f, 0.0f);

//             // Colliders
//             {
//                 const Vec3 positions[] = {origin};
//                 const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
//                 const f32 rotations[] = {Deg2Rad(0.0f)};
//                 Vec4 color = LPURPLE;
//                 color.a = 0.16f;
//                 const Vec4 colors[] = {color};

//                 for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
//                     Entity* entity = AllocateEntity();
//                     entity->pos = positions[i];
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
//                     entity->color = colors[i];
//                     entity->scale = Vec3Lit(5.0f, 5.0f, 5.0f);
//                     entity->v = velocities[i];
//                     FlagOn(entity->flags, EFLAG_COLLIDER);
//                     FlagOn(entity->flags, EFLAG_MOB);
//                     entities[size] = entity->id;
//                 }
//             }

//             MOBS_SIZE = size;
//             Assert(MOBS_SIZE < MAX_ENTITY_BATCH);
//         }

//         // TARGETS init
//         {
//             EntityId* entities = TARGETS;
//             u32 size = 0;

//             const Vec3 origin = Vec3Lit(-50.0f, 0.0f, 30.0f);

//             {
//                 const Vec3 positions[] = {origin};
//                 const Vec3 velocities[] = {Vec3Lit(3.0f, 10.0f, -6.0f)};
//                 const f32 rotations[] = {Deg2Rad(0.0f)};
//                 Vec4 color = LRED;
//                 color.a = 0.16f;
//                 const Vec4 colors[] = {color};

//                 for (u32 i = 0; i < ArraySizeOf(positions); ++i, ++size) {
//                     Entity* entity = AllocateEntity();
//                     entity->pos = positions[i];
//                     entity->mesh = GetMesh(M_ICOSAHEDRON);
//                     entity->orient = Mat4Rotate(rotations[i], Vec3Lit(0.0f, 1.0f, 0.0f));
//                     entity->color = colors[i];
//                     entity->scale = Vec3Lit(5.0f, 5.0f, 5.0f);
//                     entity->v = velocities[i];
//                     FlagOn(entity->flags, EFLAG_COLLIDER);
//                     entities[++size] = entity->id;
//                     Assert(size < MAX_ENTITY_BATCH);
//                 }
//             }

//             TARGETS_SIZE = size;
//             Assert(TARGETS_SIZE < ArraySizeOf(TARGETS));
//         }

//         // walls init.
//         {
//             static Vec3 positions[] = {Vec3Lit(0.0f, -10.0f, 0.0f)};
//             static f32 rotations[] = {Deg2Rad(-90.0f)};
//             static Vec3 axis[] = {Vec3Lit(1.0f, 0.0f, 0.0f)};
//             Vec4 colors[] = {LORANGE};

//             EntityId* entities = WALLS;
//             u32* size = &WALLS_SIZE;
//             for (u32 i = 0; i < ArraySizeOf(positions); ++i) {
//                 Entity* entity = AllocateEntity();
//                 entity->pos = positions[i];
//                 entity->mesh = GetMesh(M_QUAD);
//                 entity->orient = Mat4Rotate(rotations[i], axis[i]);
//                 entity->color = colors[i];
//                 entity->scale = Vec3Lit(10000.0f, 10000.0f, 10000.0f);
//                 entities[((*size)++) + i] = entity->id;
//             }
//         }
//     }

//     // Update logic.
//     {
//         Entity* player = GetEntity(playerid);

//         // Camera movement.
//         {
//             static const u8 GAMEPAD_ID = 0;
//             static bool FREECAM = false;

//             if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
//                 FREECAM = !FREECAM;
//             }

//             // Left stick movement.
//             {
//                 static const f32 WALK_VEL = 20.0f;
//                 static const f32 RUN_VEL = 2.0f * WALK_VEL;

//                 f32 vel = WALK_VEL;
//                 if (IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
//                     vel = RUN_VEL;
//                 }

//                 f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X);
//                 f32 y = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y);
//                 Vec3 dir_model = Vec3Lit(x, 0.0f, y);

//                 Vec3 bodyorient = vieworient;
//                 // If not freecam, then limiting pitch to 0 will do fps style movement along current plane.
//                 bodyorient.pitch = FREECAM ? bodyorient.pitch : 0.0f;
//                 Mat4 rotation = Mat4RotateEuler(bodyorient);

//                 Vec3 dir_world = Vec3Transform(dir_model, rotation);
//                 Vec3 mov_world = Vec3Scale(dir_world, vel * dt);
//                 viewpos = Vec3Add(viewpos, mov_world);
//                 player->pos = viewpos;
//             }

//             // Right stick movement.
//             {
//                 f32 x = GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_X);
//                 f32 y = -1.0f * GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_RIGHT_Y);
//                 if (x != 0.0f || y != 0.0f) {
//                     static const rad VEL = Deg2Rad(90.0f);

//                     // rotate orient.
//                     vieworient.yaw += -1.0f * x * VEL * dt;
//                     vieworient.pitch += y * VEL * dt;
//                     // Clamp pitch to avoid looking too high/low which would sign flip things.
//                     vieworient.pitch = f32clamp(vieworient.pitch, Deg2Rad(-89.0f), Deg2Rad(89.0f));
//                     player->orient = Mat4RotateEuler(vieworient);
//                 }
//             }
//         }

//         // Scene3 update.
//         {
//             {
//                 EntityManager* em = GetEntityManager();
//                 u32 esize = ArraySizeOf(em->entities);
//                 for (u32 i = 0; i < esize; ++i) {
//                     Entity* e = GetEntity(i);
//                     if (!TestFlag(e->flags, EFLAG_ACTIVE)) {
//                         continue;
//                     }

//                     if (TestFlag(e->flags, EFLAG_MOB)) {
//                         if (e->hit) {
//                             e->hit_timer += dt;
//                             if (e->hit_timer >= 1.2f) {
//                                 e->hit = false;
//                                 e->hit_timer = 0.0f;
//                             }
//                         }

//                         // Mob Movement.
//                         // TODO: You could imagine something much more intelligent like some
//                         // path finding/planner code to drive better interactions.
//                         {
//                             static const f32 SPEED = 10.0f;
//                             static const f32 ANGULAR_SPEED = Deg2Rad(15.0f);

//                             Vec3 wishdir = Vec3Normalize(Vec3Sub(player->pos, e->pos));
//                             Vec3 dir = Vec3Normalize(Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), e->orient));
//                             f32 diff = Vec3Angle(dir, wishdir);
//                             if (diff != 0.0f) {
//                                 Vec3 axis = Vec3Cross(dir, wishdir);

//                                 i32 flip = diff > 0.0f ? 1.0f : -1.0f;
//                                 diff = f32abs(diff);
//                                 f32 to_rotate = f32min(diff, ANGULAR_SPEED * dt);

//                                 Mat4 rot = Mat4Rotate(flip * to_rotate, axis);
//                                 dir = Vec3Normalize(Vec3Transform(dir, rot));
//                                 e->orient = Mat4PointAt(dir);
//                             }

//                             bool should_move = !e->hit && (diff <= ANGULAR_SPEED * 1.4f);
//                             e->v = Vec3Scale(Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), e->orient), SPEED *
//                             should_move); e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
//                         }
//                     }

//                     if (TestFlag(e->flags, EFLAG_PROJECTILE)) {
//                         e->pos = Vec3Add(e->pos, Vec3Scale(e->v, dt));
//                     }

//                     if (TestFlag(e->flags, EFLAG_PLAYER_CONTROLLED)) {
//                         if (IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
//                             Entity* projectile = AllocateEntity();
//                             projectile->pos = player->pos;
//                             projectile->mesh = GetMesh(M_ICOSPHERE);
//                             projectile->color = LPURPLE;
//                             projectile->scale = Vec3Lit(0.3f, 0.3f, 0.3f);
//                             FlagOn(projectile->flags, EFLAG_PROJECTILE);
//                             FlagOn(projectile->flags, EFLAG_COLLIDER);
//                             projectile->v =
//                                 Vec3Scale(Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), player->orient), 200.0f);
//                         }
//                     }
//                 }

//                 // Draw.
//                 for (u32 i = 0; i < ArraySizeOf(em->entities); ++i) {
//                     Entity* e = GetEntity(i);
//                     if (!TestFlag(e->flags, EFLAG_ACTIVE) || !TestFlag(e->flags, EFLAG_DRAW)) {
//                         continue;
//                     }

//                     Vec4 color = e->color;
//                     if (TestFlag(e->flags, EFLAG_MOB) && e->hit) {
//                         static const f32 FREQ = 2.5f;
//                         f32 t = 0.5f * (1.0f + f32sin(2.0f * LPI * FREQ * e->hit_timer));
//                         color.r = f32lerp(color.r, LRED.r, t);
//                         color.g = f32lerp(color.g, LRED.g, t);
//                         color.b = f32lerp(color.b, LRED.b, t);
//                         color.a = f32lerp(color.a, LRED.a, t);
//                     }

//                     Mat4 scale = Mat4ScaleV(e->scale);
//                     Mat4 translate = Mat4TranslateV(e->pos);
//                     Mat4 model = Mat4Multiply(translate, Mat4Multiply(e->orient, scale));
//                     AddCmdOld(e->mesh, model, color);
//                     DrawCmd* cmd2 = AddCmdOld(e->mesh, model, color);
//                     cmd2->wireframe = true;

//                     if (gstate->debug && TestFlag(e->flags, EFLAG_COLLIDER)) {
//                         Sphere sphere = EntityBoundingSphere(e);
//                         DrawSphereWf(sphere.c, sphere.r, LYELLOW);
//                     }
//                 }
//             }
//         }
//     }

//     // Drawing
//     {
//         // Begin.
//         {
//             BeginDrawing();
//             glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
//             glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//             glBindVertexArray(VAO);
//         }

//         Mat4 view = GetCameraMat(viewpos, vieworient);
//         Mat4 proj = Mat4Perspective(Deg2Rad(45.0f), (f32)GetScreenWidth() / (f32)GetScreenHeight(), 0.1f, 1000.0f);

//         // Enable/Disable culling.
//         // TODO: Make this nicer at some point. Useful for debugging.
//         {
//             if (gstate->debug) {
//                 glDisable(GL_CULL_FACE);
//             } else {
//                 glEnable(GL_CULL_FACE);
//                 glCullFace(GL_BACK);
//                 glFrontFace(GL_CCW);
//             }
//         }

//         // Use shader + setup norm + view + proj matrix.
//         LShader shader = GetShaderOld();
//         {
//             glUseProgram(shader.pid);
//             glUniformMatrix4fv(shader.vview_loc, 1, GL_FALSE, (const f32*)&view.m);
//             glUniformMatrix4fv(shader.vproj_loc, 1, GL_FALSE, (const f32*)&proj.m);
//         }

//         for (u32 i = 0; i < CBUFFER_SIZE; ++i) {
//             DrawCmd* cmd = &CBUFFER[i];
//             glUniformMatrix4fv(shader.vmodel_loc, 1, GL_FALSE, (const f32*)&cmd->modelm.m);
//             glUniform4fv(shader.vucolor_loc, 1, (const f32*)&cmd->color.f);
//             if (cmd->wireframe) {
//                 glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//             }
//             glDrawElements(GL_TRIANGLES, cmd->meshid.ntriangles * 3, GL_UNSIGNED_INT,
//                            (void*)(cmd->meshid.ibufferidx * sizeof(GLuint)));
//             if (cmd->wireframe) {
//                 glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//             }
//         }

//         EndDrawing();
//         CmdBufferZero();  // Maybe consider moving this to be at the top of update func.
//     }
// }

// Keep this around for debugging for a bit.
// Mat4 vieworientm = Mat4RotateEuler(vieworient);
// Vec3 viewforward = Vec3Transform(Vec3Lit(0.0f, 0.0f, -1.0f), vieworientm);
// // // Vec3 viewforward = Vec3Lit(f32cos(vieworient.yaw) * f32cos(vieworient.pitch), f32sin(vieworient.pitch),
// // //                            -f32sin(vieworient.yaw) * f32cos(vieworient.pitch));
// Mat4 view2 = Mat4LookAt(viewpos, Vec3Add(viewpos, viewforward), Vec3Lit(0.0f, 1.0f, 0.0f));
// Assert(Mat4Eq(view, view2));

// // Collisions Init.
// {
//     static const Vec3 row_margin = Vec3Lit(0.0f, 0.0f, 5.0f);
//     static const Vec3 col_margin = Vec3Lit(-10.0f, 0.0f, 0.0f);
//     static const Vec3 origin = Vec3Lit(20.0f, 3.0f, 15.0f);

//     EntityId* entities = COLLISIONS;
//     u32 size = 0;
//     u32 ncols = 0;

//     // Test all face-face combinations.
//     {
//         static Vec3 axis[] = {
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//         };
//         static f32 rotations[] = {
//             Deg2Rad(0.0f),   Deg2Rad(0.0f),    //
//             Deg2Rad(0.0f),   Deg2Rad(90.0f),   //
//             Deg2Rad(0.0f),   Deg2Rad(180.0f),  //
//             Deg2Rad(0.0f),   Deg2Rad(270.0f),  //
//             Deg2Rad(0.0f),   Deg2Rad(90.0f),   //
//             Deg2Rad(0.0f),   Deg2Rad(-90.0f),  //

//             Deg2Rad(90.0f),  Deg2Rad(0.0f),    //
//             Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
//             Deg2Rad(90.0f),  Deg2Rad(180.0f),  //
//             Deg2Rad(90.0f),  Deg2Rad(270.0f),  //
//             Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
//             Deg2Rad(90.0f),  Deg2Rad(-90.0f),  //

//             Deg2Rad(180.0f), Deg2Rad(0.0f),    //
//             Deg2Rad(180.0f), Deg2Rad(90.0f),   //
//             Deg2Rad(180.0f), Deg2Rad(180.0f),  //
//             Deg2Rad(180.0f), Deg2Rad(270.0f),  //
//             Deg2Rad(180.0f), Deg2Rad(90.0f),   //
//             Deg2Rad(180.0f), Deg2Rad(-90.0f),  //

//             Deg2Rad(180.0f), Deg2Rad(0.0f),    //
//             Deg2Rad(270.0f), Deg2Rad(90.0f),   //
//             Deg2Rad(270.0f), Deg2Rad(180.0f),  //
//             Deg2Rad(270.0f), Deg2Rad(270.0f),  //
//             Deg2Rad(270.0f), Deg2Rad(90.0f),   //
//             Deg2Rad(270.0f), Deg2Rad(-90.0f),  //

//             Deg2Rad(90.0f),  Deg2Rad(0.0f),    //
//             Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
//             Deg2Rad(90.0f),  Deg2Rad(180.0f),  //
//             Deg2Rad(90.0f),  Deg2Rad(270.0f),  //
//             Deg2Rad(90.0f),  Deg2Rad(90.0f),   //
//             Deg2Rad(90.0f),  Deg2Rad(-90.0f),  //

//             Deg2Rad(-90.0f), Deg2Rad(0.0f),    //
//             Deg2Rad(-90.0f), Deg2Rad(90.0f),   //
//             Deg2Rad(-90.0f), Deg2Rad(180.0f),  //
//             Deg2Rad(-90.0f), Deg2Rad(270.0f),  //
//             Deg2Rad(-90.0f), Deg2Rad(90.0f),   //
//             Deg2Rad(-90.0f), Deg2Rad(-90.0f),  //

//         };

//         {
//             Vec3 pos = Vec3Add(origin, Vec3Scale(col_margin, ncols));
//             ++ncols;

//             for (u32 i = 0; i < ArraySizeOf(rotations); i += 2, size += 2) {
//                 {
//                     u32 j = i;
//                     Entity* entity = AllocateEntity();
//                     entity->pos = pos;
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[j], axis[j]);
//                     entity->color = LBLUE;  //: LORANGE;
//                     entity->color.a = 0.5f;
//                     entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);  // 1.0f;
//                     entities[size + 0] = entity->id;
//                 }
//                 {
//                     u32 j = i + 1;
//                     Entity* entity = AllocateEntity();
//                     entity->pos = Vec3Add(pos, row_margin);
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[j], axis[j]);
//                     entity->color = LORANGE;
//                     entity->color.a = 0.5f;
//                     entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
//                     entities[size + 1] = entity->id;
//                 }

//                 {
//                     Entity* e0 = GetEntity(entities[size + 0]);
//                     Entity* e1 = GetEntity(entities[size + 1]);

//                     // Collide in half sec.
//                     // NOTE: this will just collide the centers, but actual collision might occur sooner.
//                     Vec3 v = Vec3Sub(e0->pos, e1->pos);
//                     f32 sec = 3.0f;
//                     v = Vec3Scale(v, 1.0f / sec);
//                     e1->v = v;
//                 }

//                 pos = Vec3Add(pos, Vec3Scale(row_margin, 2.0f));
//             }
//         }

//         // Col 1 rotation comb + obj B on top.
//         {
//             Vec3 pos = Vec3Add(origin, Vec3Scale(col_margin, ncols));
//             ++ncols;

//             for (u32 i = 0; i < ArraySizeOf(rotations); i += 2, size += 2) {
//                 {
//                     u32 j = i;
//                     Entity* entity = AllocateEntity();
//                     entity->pos = pos;
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[j], axis[j]);
//                     entity->color = LBLUE;  //: LORANGE;
//                     entity->color.a = 0.5f;
//                     entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);  // 1.0f;
//                     entities[size + 0] = entity->id;
//                 }
//                 {
//                     u32 j = i + 1;
//                     Entity* entity = AllocateEntity();
//                     entity->pos = Vec3Add(pos, Vec3Lit(0.0f, 5.0f, 0.0f));
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[j], axis[j]);
//                     entity->color = LORANGE;
//                     entity->color.a = 0.5f;
//                     entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
//                     entities[size + 1] = entity->id;
//                 }

//                 {
//                     Entity* e0 = GetEntity(entities[size + 0]);
//                     Entity* e1 = GetEntity(entities[size + 1]);

//                     // Collide in half sec.
//                     // NOTE: this will just collide the centers, but actual collision might occur sooner.
//                     Vec3 v = Vec3Sub(e0->pos, e1->pos);
//                     f32 sec = 3.0f;
//                     v = Vec3Scale(v, 1.0f / sec);
//                     e1->v = v;
//                 }

//                 pos = Vec3Add(pos, Vec3Scale(row_margin, 2.0f));
//             }
//         }
//     }

//     // Test some basic cube rotations.
//     {
//         static Vec3 axis[] = {
//             // This first case is edge on edge collision.
//             // TODO: The collision detects, but the normal is not right. need to figure out
//             // how to handle case of parallel edges. How to determine what the axis of separation is
//             // if edges are parallel and on a line with eachother?
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.3f, 0.5f),                //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.707106781f, -0.707106781f, 0),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f),  //

//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),  //

//             // Reverse.
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //

//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //

//             Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//             Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 1.0f, 0.0f),  //
//         };
//         static f32 rotations[] = {
//             Deg2Rad(45.0f), Deg2Rad(45.0f),    //
//             Deg2Rad(45.0f), Deg2Rad(45.0f),    //
//             Deg2Rad(53.0f), Deg2Rad(53.0f),    //
//             Deg2Rad(-72.0f), Deg2Rad(-72.0f),  //
//             Deg2Rad(0.1f), Deg2Rad(0.1f),      //
//             Deg2Rad(-12.0f), Deg2Rad(-12.0f),  //

//             Deg2Rad(0.0f), Deg2Rad(120.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(125.264),  //

//             Deg2Rad(0.0f), Deg2Rad(45.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(53.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(-72.0f),  //
//             Deg2Rad(0.0f), Deg2Rad(0.1f),    //
//             Deg2Rad(0.0f), Deg2Rad(-12.0f),  //

//             Deg2Rad(0.0f), Deg2Rad(45.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(53.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(-72.0f),  //
//             Deg2Rad(0.0f), Deg2Rad(0.1f),    //
//             Deg2Rad(0.0f), Deg2Rad(-12.0f),  //

//             Deg2Rad(0.0f), Deg2Rad(45.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(53.0f),   //
//             Deg2Rad(0.0f), Deg2Rad(-72.0f),  //
//             Deg2Rad(0.0f), Deg2Rad(0.1f),    //
//             Deg2Rad(0.0f), Deg2Rad(-12.0f),  //

//             // Reverse
//             Deg2Rad(45.0f), Deg2Rad(0.0f),   //
//             Deg2Rad(53.0f), Deg2Rad(0.0f),   //
//             Deg2Rad(-72.0f), Deg2Rad(0.0f),  //
//             Deg2Rad(0.1f), Deg2Rad(0.0f),    //
//             Deg2Rad(-12.0f), Deg2Rad(0.0f),  //

//             Deg2Rad(45.0f), Deg2Rad(0.0f),   //
//             Deg2Rad(53.0f), Deg2Rad(0.0f),   //
//             Deg2Rad(-72.0f), Deg2Rad(0.0f),  //
//             Deg2Rad(0.1f), Deg2Rad(0.0f),    //
//             Deg2Rad(-12.0f), Deg2Rad(0.0f),  //

//             Deg2Rad(45.0f), Deg2Rad(0.0f),   //
//             Deg2Rad(53.0f), Deg2Rad(0.0f),   //
//             Deg2Rad(-72.0f), Deg2Rad(0.0f),  //
//             Deg2Rad(0.1f), Deg2Rad(0.0f),    //
//             Deg2Rad(-12.0f), Deg2Rad(0.0f),  //
//         };

//         {
//             Vec3 pos = Vec3Add(origin, Vec3Scale(col_margin, ncols));
//             ++ncols;

//             for (u32 i = 0; i < ArraySizeOf(rotations); i += 2, size += 2) {
//                 {
//                     u32 j = i;
//                     Entity* entity = AllocateEntity();
//                     entity->pos = pos;
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[j], axis[j]);
//                     entity->color = LBLUE;  //: LORANGE;
//                     entity->color.a = 0.5f;
//                     entity->scale = Vec3Lit(3.0f, 3.0f, 3.0f);  // 1.0f;
//                     entities[size + 0] = entity->id;
//                 }
//                 {
//                     u32 j = i + 1;
//                     Entity* entity = AllocateEntity();
//                     entity->pos = Vec3Add(pos, row_margin);
//                     entity->mesh = GetMesh(M_CUBE);
//                     entity->orient = Mat4Rotate(rotations[j], axis[j]);
//                     entity->color = LORANGE;
//                     entity->color.a = 0.5f;
//                     entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
//                     entities[size + 1] = entity->id;
//                 }

//                 {
//                     Entity* e0 = GetEntity(entities[size + 0]);
//                     Entity* e1 = GetEntity(entities[size + 1]);

//                     // Collide in half sec.
//                     // NOTE: this will just collide the centers, but actual collision might occur sooner.
//                     Vec3 v = Vec3Sub(e0->pos, e1->pos);
//                     f32 sec = 3.0f;
//                     v = Vec3Scale(v, 1.0f / sec);
//                     e1->v = v;
//                 }

//                 pos = Vec3Add(pos, Vec3Scale(row_margin, 2.0f));
//             }
//         }
//     }

//     COLLISIONS_SIZE = size;
//     Assert(COLLISIONS_SIZE < MAX_ENTITY_BATCH);
// }
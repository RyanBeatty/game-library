////////////////////////////////////////////////////
// S:UI

// typedef enum {
//     UI_SIZE_TEXT = 0,
//     UI_SIZE_PCT_OF_PARENT,
//     UI_SIZE_PIXEL,
// } UISizeKind;

// #define UISizeText() ((UISize){.kind = UI_SIZE_TEXT})
// #define UISizePctParent(vv) ((UISize){.kind = UI_SIZE_PCT_OF_PARENT, .value = (vv)})
// #define UISizePx(px) ((UISize){.kind = UI_SIZE_PIXEL, .value = (px)})

// typedef struct {
//     UISizeKind kind;
//     f32 value;
// } UISize;

// typedef enum {
//     UI_FLAG_NONE = 0,
//     UI_FLAG_CLICKABLE = 1 << 0,
//     UI_FLAG_HIDE = 1 << 1,  // Note that by default we draw boxes.
//     // i.e. a button. I kind of don't remember why I added this, but for a button at the moment you need both this +
//     // CLICKABLE. This definitely controls if collision test will run and set the clicked action result. I think
//     // CLICKABLE is more an aesthetic thing about how the button is drawn.
//     UI_FLAG_ABLE_TO_BE_CLICKED = 1 << 2,
// } UIFlag;

// #define UITestFlag(flags, flag) ((flags) & (flag))

// typedef enum {
//     UI_TEXT_ALIGN_X_NONE = 0,
//     UI_TEXT_ALIGN_X_CENTER,
//     UI_TEXT_ALIGN_X_LEFT,
// } UITextAlignX;
// typedef enum {
//     UI_TEXT_ALIGN_Y_NONE = 0,
//     UI_TEXT_ALIGN_Y_MIDDLE,
//     UI_TEXT_ALIGN_Y_BOTTOM,
// } UITextAlignY;

// typedef struct {
//     UITextAlignX x;
//     UITextAlignY y;
// } UITextAlign;

// typedef struct {
//     UITextAlign* data;
//     isize len;
//     isize cap;
// } UITextAlignArr;

// typedef struct {
//     LFont* data;
//     isize len;
//     isize cap;
// } LFontArr;

// typedef enum {
//     UI_LAYOUT_DOWN = 0,
//     UI_LAYOUT_RIGHT,
// } UILayoutDirection;

// typedef struct {
//     UILayoutDirection* data;
//     isize len;
//     isize cap;
// } UILayoutDirectionArr;

// typedef struct {
//     Vec4 color_bg;
//     Vec4 color_bg_hover;
//     Vec4 color_bg_down;
//     Vec4 color_bg_unclickable;
//     Vec4 color_text;

//     UITextAlign align;
// } UIBoxStyle;

// typedef struct {
//     isize id;
// } UIBoxId;

// static UIBoxId NULL_UIBOXID = {.id = 0};
// #define UIBoxIdEq(a, b) ((a).id == (b).id)
// #define UIBoxIdIsNull(a) (UIBoxIdEq(a, NULL_UIBOXID))

// typedef struct {
//     UIBoxId id;

//     // The box's center should be in world position.
//     // TODO: Longer term I should probably think about just storing an offset to parent and a dimension
//     // Then you recalculate the world position each frame from the parent (root of tree will get actual screen top
//     left
//     // as position coordinate). But because we recreate all boxes for the UI each frame, might not be necessary to do
//     // this at the moment.
//     AABB2D box;

//     // Note that this should be a *negative* offset from the top left. i.e. top left + this = next layout position.
//     // TODO: I should potentially change to make everything based off of the bottom left as I think the math will be
//     // less confusing.
//     Vec2 cursor_top_left_offset;            // Where the next point to layout a ui element inside of the box is.
//     Vec2 cursor_top_left_offset_prev_line;  // Where the cursor last was *on the previous line*. Useful if we need to
//                                             // reset to draw things on same line.

//     Str8 text;
//     TextBox textbox;  // Not positioned. i.e. the center here holds no meaning other than an offset.

//     UIBoxStyle style;  // Contains color info to be used when rendering the box.
// } UIBox;

// #define UIBoxGetCursor(bbox) (Vec2Add(AABB2DTopLeft((bbox)->box), (bbox)->cursor_top_left_offset))

// typedef struct {
//     UIBox* data;
//     isize len;
//     isize cap;
// } UIBoxArr;

// typedef enum {
//     UI_THEME_NONE = 0,

//     UI_THEME_WINDOW_BG_COLOR,

//     UI_THEME_BUTTON_BG_COLOR,
//     UI_THEME_BUTTON_BG_HOVER_COLOR,
//     UI_THEME_BUTTON_BG_DOWN_COLOR,
//     UI_THEME_BUTTON_BG_UNCLICKABLE_COLOR,
//     UI_THEME_BUTTON_TEXT_COLOR,

//     UI_THEME_LABEL_BG_COLOR,
//     UI_THEME_LABEL_TEXT_COLOR,

//     UI_THEME_COUNT,
// } UIThemeKind;

// typedef struct {
//     UIThemeKind idx;
//     Vec4 color_prev;
// } UIThemeColorOverride;

// typedef struct {
//     UIThemeColorOverride* data;
//     isize len;
//     isize cap;
// } UIThemeColorOverrideArr;

// typedef struct {
//     Vec4 colors[UI_THEME_COUNT];

//     UIThemeColorOverrideArr color_overrides;

//     UITextAlign align;
// } UITheme;

// #define ColorHover(color) (Lighten((color), 0.15f))
// #define ColorDown(color) (Darken((color), 0.12f))
// #define ColorUnclickable(color) (Darken((color), 0.12f))

// typedef struct {
//     LFontArr fonts;
//     Vec2Arr spacings;
//     // The *minimum* amount of pixel space from the left and right edges of a box to the left and right edges of the
//     // text (and similar for top and bottom). if the text width + padding >= parent container, will clamp to parent
//     // container (though text might overflow)
//     Vec2Arr paddings;
//     UILayoutDirectionArr layout_dirs;
//     UIBoxArr parents;

//     UITheme theme;

//     UIBoxId next_id;
//     UIBoxId active_id;
//     UIBoxId active_id_prev;

//     GameInput* input;  // Reset each frame.
//     GameButtonType mouse_click_button;
// } UIState;

// typedef struct {
//     bool clicked;
//     bool down;
//     bool hover;
// } UIAction;

// #define UIGetFont(ui) (ArrTail(&(ui)->fonts))
// #define UIPushFont(ui, ffont) (ArrPush(&(ui)->fonts, ffont))
// #define UIPopFont(ui) (ArrPop(&(ui)->fonts))
// #define UIFont(ui, ffont) Scoped(UIPushFont(ui, ffont), UIPopFont(ui))

// #define UIPushSpacing(ui, sspacing) (ArrPush(&(ui)->spacings, sspacing))
// #define UIPushPadding(ui, padding) (ArrPush(&(ui)->paddings, (padding)))
// #define UIPopPadding(ui) (ArrPop(&(ui)->paddings))
// #define UIPadding(ui, padding) Scoped(UIPushPadding(ui, padding), UIPopPadding(ui))
// #define UIGetPadding(ui) (ArrTail(&(ui)->paddings))

// #define UIGetLayoutDirection(ui) (ArrTail(&(ui)->layout_dirs))
// #define UIPushLayoutDirection(ui, llayout) (ArrPush(&(ui)->layout_dirs, (llayout)))
// #define UIPopLayoutDirection(ui) (ArrPop(&(ui)->layout_dirs))
// #define UILayoutDirection(ui, llayout) Scoped(UIPushLayoutDirection(ui, llayout), UIPopLayoutDirection(ui))

// #define UIGetThemeColor(ui, uikind) ((ui)->theme.colors[(uikind)])
// LCORE void UIPushThemeColor(UIState* ui, UIThemeKind kind, Vec4 color) {
//     UIThemeColorOverride override = {.idx = kind, .color_prev = ui->theme.colors[kind]};
//     ArrPush(&ui->theme.color_overrides, override);
//     ui->theme.colors[kind] = color;
// }
// LCORE void UIPopThemeColor(UIState* ui) {
//     UIThemeColorOverride override = ArrPop(&ui->theme.color_overrides);
//     ui->theme.colors[override.idx] = override.color_prev;
// }
// #define UIThemeColor(ui, uikind, color) Scoped(UIPushThemeColor((ui), (uikind), (color)), UIPopThemeColor((ui)))

// #define UIGetThemeTextAlign(ui) ((ui)->theme.align)

// #define UIGetParent(ui) (ArrTailR(&(ui)->parents))
// #define UIPushParent(ui, pparent) (ArrPush(&(ui)->parents, (pparent)))
// #define UIPopParent(ui) (ArrPop(&(ui)->parents))
// #define UIParent(ui, pparent) Scoped(UIPushParent((ui), (pparent)), UIPopParent((ui)))

// static UIBoxId UIStateNextId(UIState* ui) {
//     UIBoxId id = ui->next_id;
//     ui->next_id = (UIBoxId){.id = id.id + 1};
//     return id;
// }

// // Set the current parent's cursor offset to pos.
// // REMINDER THAT THE Y COORDINATE HERE SHOULD BE A NEGATIVE OFFSET FROM THE TOP LEFT
// LCORE void UISetCursor(UIState* ui, Vec2 pos) {
//     UIBox* root = UIGetParent(ui);
//     // TODO: We should potentially add some checks or clamping around setting a cursor outside of the parent's
//     window. root->cursor_top_left_offset = pos;
// }

// LCORE Vec2 UIGetCursor(UIState* ui) {
//     UIBox* root = UIGetParent(ui);
//     return root->cursor_top_left_offset;
// }

// static void UIStateInit(UIState* ui, Arena* perm, f32 width, f32 height, GameInput* input,
//                         GameButtonType mouse_click_button) {
//     InitArr(&ui->spacings, perm, Vec2, 1);

//     InitArr(&ui->paddings, perm, Vec2, 2);
//     UIPushPadding(ui, Vec2Zero());

//     InitArr(&ui->fonts, perm, LFont, 3);
//     UIPushFont(ui, GetLFontClosest(10.0f));
//     InitArr(&ui->layout_dirs, perm, UILayoutDirection, 2);
//     UIPushLayoutDirection(ui, UI_LAYOUT_DOWN);

//     // Make sure first id is not the null id.
//     ui->next_id = (UIBoxId){.id = NULL_UIBOXID.id + 1};

//     InitArr(&ui->parents, perm, UIBox, 10);
//     AABB2D parent_box = AABB2DFromMinMax(Vec2Lit(0, 0), Vec2Lit(width, height));
//     UIBox root_parent = {.box = parent_box, .cursor_top_left_offset = Vec2Zero(), .id = UIStateNextId(ui)};
//     UIPushParent(ui, root_parent);

//     {
//         UITheme theme_default = {0};
//         InitArr(&theme_default.color_overrides, perm, UIThemeColorOverride, 10);

//         for (isize i = 0; i < ArraySizeOf(theme_default.colors); ++i) {
//             theme_default.colors[i] = LBLACK;
//         }

//         theme_default.colors[UI_THEME_WINDOW_BG_COLOR] = COLORS[0];

//         theme_default.colors[UI_THEME_BUTTON_BG_COLOR] = COLORS[3];
//         theme_default.colors[UI_THEME_BUTTON_BG_HOVER_COLOR] = ColorHover(COLORS[3]);
//         theme_default.colors[UI_THEME_BUTTON_BG_DOWN_COLOR] = ColorDown(COLORS[3]);
//         theme_default.colors[UI_THEME_BUTTON_BG_UNCLICKABLE_COLOR] = ColorUnclickable(COLORS[3]);
//         theme_default.colors[UI_THEME_BUTTON_TEXT_COLOR] = LBLACK;

//         theme_default.colors[UI_THEME_LABEL_BG_COLOR] = Vec4Lit(0.0f, 0.0f, 0.0f, 0.0f);
//         theme_default.colors[UI_THEME_LABEL_TEXT_COLOR] = LBLACK;

//         ui->theme = theme_default;
//     }

//     ui->input = input;
//     ui->mouse_click_button = mouse_click_button;
// }

// static void UIBegin(UIState* ui, Arena* perm, GameInput* input, GameButtonType mouse_click_button, f32 width,
//                     f32 height, LShader shader, RenderPassLoadAction load_action, RenderTarget target) {
//     Mat4 view = Mat4Identity();
//     Mat4 proj = Mat4Ortho(0, width, 0, height, -1.0f, 1.0f);

//     BeginShaderPass(shader, load_action, view, proj, target);
//     UIStateInit(ui, perm, width, height, input, mouse_click_button);
// }

// static void UIEnd(UIState* ui) { EndShaderPass(); }

// #define UI2D(ui, perm, input, mouse_click_button, width, height, shader, load_action, target)
//     Scoped(UIBegin((ui), (perm), (input), (mouse_click_button), (width), (height), (shader), (load_action),
//     (target)),
//            UIEnd((ui)))

// static UIBox MakeUIBox(UIState* ui, UISize width, UISize height, Str8 text, UIBoxStyle style) {
//     UIBox* parent = UIGetParent(ui);
//     Vec2 top_left = UIBoxGetCursor(parent);
//     LFont font = UIGetFont(ui);

//     Vec2 parent_dims = AABB2DDims(parent->box);

//     TextBox textbox = LFontMeasureStr8Quad(font, text);
//     Vec2 textbox_dims = AABB2DDims(textbox.aabb);

//     Vec2 padding = UIGetPadding(ui);
//     UISize semantic_sizes[] = {width, height};
//     Vec2 resolved_dims = {0};
//     for (isize i = 0; i < ArraySizeOf(semantic_sizes); ++i) {
//         switch (semantic_sizes[i].kind) {
//             case UI_SIZE_TEXT: {
//                 // TODO: Should I clamp this parent dims?
//                 resolved_dims.f[i] = textbox_dims.f[i] + padding.f[i];
//                 break;
//             }
//             case UI_SIZE_PCT_OF_PARENT: {
//                 resolved_dims.f[i] = semantic_sizes[i].value * parent_dims.f[i];
//                 break;
//             }
//             case UI_SIZE_PIXEL: {
//                 // Do I need to clamp this to parent dims?
//                 resolved_dims.f[i] = semantic_sizes[i].value;
//                 break;
//             }
//             default: {
//                 PlatformLog(LLOG_ERROR, "UNKNOWN UI Size Kind: %d", semantic_sizes[i].kind);
//                 Assert(0);
//             }
//         }
//     }

//     Vec2 box_min = Vec2Lit(top_left.x, top_left.y - resolved_dims.y);
//     Vec2 box_max = Vec2Lit(top_left.x + resolved_dims.x, top_left.y);

//     AABB2D aabb = AABB2DFromMinMax(box_min, box_max);
//     UIBox box = {.box = aabb, .text = text, .textbox = textbox, .id = UIStateNextId(ui)};

//     // Update parent cursor pos.
//     {
//         Vec2 box_dims = AABB2DDims(box.box);
//         parent->cursor_top_left_offset.x = f32min(parent->cursor_top_left_offset.x + box_dims.x, parent_dims.x);
//         parent->cursor_top_left_offset_prev_line = parent->cursor_top_left_offset;
//         // Now we end the line, and advance to next line.
//         // Because our Y offsets are a negative offset (since we offset from the top left down), check with parent
//         // negative.
//         parent->cursor_top_left_offset.y = f32max(parent->cursor_top_left_offset.y - box_dims.y, -parent_dims.y);
//         parent->cursor_top_left_offset.x = 0;  // Reset to paren't left edge because we are new line.
//     }

//     // UILayoutDirection dir = UIGetLayoutDirection(ui);
//     // // TODO: We might want to modify this to be more like Imgui. Layout happens from top to bottom + left to right
//     // // (though maybe we should consider for our purposes bottom to top?). When Imgui adds a box, it increments the
//     X
//     // // value as well as the Y value. once it detects X has hit the end of a line (guessing its the width of the
//     // parent),
//     // // then it resets X offset to be 0 (i.e. parent's left edge coord) as well as increment Y by the new box
//     height
//     // for
//     // // the next line. At the moment, we always treat X offset as 0 from the parent, so hard to layout things in
//     the
//     // same
//     // // line without switching layout direction. Feel like it might be easier to layout right with just a few calls
//     to
//     // // UISameLine() instead of having to push the direction change each time
//     // switch (dir) {
//     //     case UI_LAYOUT_DOWN: {
//     //         parent->cursor_top_left_offset.y -= box_dims.y;
//     //         parent->cursor_top_left_offset.y = f32max(parent->cursor_top_left_offset.y, -parent_dims.y);
//     //         break;
//     //     }
//     //     case UI_LAYOUT_RIGHT: {
//     //         parent->cursor_top_left_offset.x += box_dims.x;
//     //         parent->cursor_top_left_offset.x = f32min(parent->cursor_top_left_offset.x, parent_dims.x);
//     //         break;
//     //     }
//     //     default: {
//     //         PlatformLog(LLOG_ERROR, "Unknown Layout Direction: %d", dir);
//     //         Assert(0);
//     //     }
//     // }

//     box.style = style;

//     return box;
// }

// static void DrawUIBoxEx(UIState* ui, UIBox* box, UIFlag flags, UIAction action) {
//     if (UITestFlag(flags, UI_FLAG_HIDE)) {
//         return;
//     }

//     // Draw backing box.
//     {
//         Vec4 box_color = box->style.color_bg;
//         if (UITestFlag(flags, UI_FLAG_ABLE_TO_BE_CLICKED)) {
//             if (UITestFlag(flags, UI_FLAG_CLICKABLE)) {
//                 if (action.down) {
//                     box_color = box->style.color_bg_down;
//                 } else if (action.hover) {
//                     box_color = box->style.color_bg_hover;
//                 }
//             } else {
//                 box_color = box->style.color_bg_unclickable;
//             }
//         }

//         DrawQuad2D(box->box.c, AABB2DWidth(box->box), AABB2DHeight(box->box), Mat4Identity(), box_color);
//     }

//     if (Str8Len(box->text)) {
//         LFont font = UIGetFont(ui);

//         Vec2 box_dims = AABB2DDims(box->box);

//         Vec2 bottom_left = AABB2DBottomLeft(box->box);
//         Vec2 textbox_center_offset = Vec2Zero();

//         switch (box->style.align.x) {
//             case UI_TEXT_ALIGN_X_LEFT: {
//                 break;
//             }
//             case UI_TEXT_ALIGN_X_CENTER: {
//                 textbox_center_offset.x = (0.5f * box_dims.x) - box->textbox.aabb.c.x;
//                 break;
//             }
//             case UI_TEXT_ALIGN_X_NONE:
//             default: {
//                 PlatformLog(LLOG_ERROR, "Unknown text align x: %d", box->style.align.x);
//                 Assert(0);
//             }
//         }

//         switch (box->style.align.y) {
//             case UI_TEXT_ALIGN_Y_BOTTOM: {
//                 break;
//             }
//             case UI_TEXT_ALIGN_Y_MIDDLE: {
//                 // TODO: this might not actually be vertically centered like how most engines do it? It seems like
//                 there
//                 // is some concept of font height which I don't want to focus on at the moment.
//                 textbox_center_offset.y = (0.5f * box_dims.y) - box->textbox.aabb.c.y;
//                 break;
//             }
//             case UI_TEXT_ALIGN_Y_NONE:
//             default: {
//                 PlatformLog(LLOG_ERROR, "Unknown text align y: %d", box->style.align.y);
//                 Assert(0);
//             }
//         }

//         textbox_center_offset = Vec2Add(textbox_center_offset, box->textbox.aabb.c);
//         Vec2 baseline_offset = Vec2Add(textbox_center_offset, box->textbox.baseline_offset);
//         Vec2 text_pos = Vec2Add(bottom_left, baseline_offset);
//         DrawStr8(font, text_pos, box->style.color_text, box->text);
//     }
// }

// // TODO: I think text alignment should come from the style potentially
// static UIBox UIBoxEx(UIState* ui, Str8 text, UIFlag flags, UISize semantic_width, UISize semantic_height,
//                      UIBoxStyle style, UIAction* action_out) {
//     UIBox box = MakeUIBox(ui, semantic_width, semantic_height, text, style);

//     GameControllerState* controller = GetController(ui->input, 0);

//     // Note that the box should be in screen space already.
//     Vec2 normal = {0};
//     f32 depth = 0.0f;
//     bool collides = TestPoint2DVsAABB2D(controller->cursor_pos, box.box, &normal, &depth);

//     UIAction action = {0};
//     // TODO: There is a potential issue here in that by the time we've checked the down action here,
//     // we might have triggered gameplay actions as well since this logic check happens after the
//     // update step. Ideally we decouple the UI logic detection/gameplay state update and the render.
//     // But they are coupled for the time being.
//     if (collides && UITestFlag(flags, UI_FLAG_CLICKABLE)) {
//         // Prevent us from activating two adjacent buttonsa t the same time. This can happen occasionally due to
//         // floating point math.
//         if (UIBoxIdIsNull(ui->active_id) || UIBoxIdEq(ui->active_id, box.id)) {
//             GameButtonState* mouse_button = &controller->buttons[ui->mouse_click_button];

//             ui->active_id = box.id;
//             action.hover = true;
//             action.clicked = mouse_button->pressed;
//             action.down = mouse_button->down;

//             // TODO: Would like to add a sound for when we are focusing an element. However that requires keeping
//             track
//             // of prev_active_id which requires maintaining UI state between frames instead of within a single frame.
//             if (action.clicked) {
//                 PlatformPlaySound(GetSound(SOUND_UI_BUTTON_CLICK), 0);
//             }

//             // PlatformLog(LLOG_INFO, "hover: %d| clicked: %d| down: %d", action.hover, action.clicked, action.down);
//         }
//     }

//     DrawUIBoxEx(ui, &box, flags, action);

//     *action_out = action;
//     return box;
// }

// static UIBox MakeUIWindow(UIState* ui, UISize width, UISize height) {
//     UIBoxStyle style = {
//         .color_bg = UIGetThemeColor(ui, UI_THEME_WINDOW_BG_COLOR),
//     };
//     UIAction action = {0};
//     UIBox box = UIBoxEx(ui, Str8Zero(), UI_FLAG_NONE, width, height, style, &action);
//     return box;
// }

// // Need to define variable and call UIParent in two steps or can run into bugs around macro expansion due to using
// // incorrect parent in MakeUIWindow for sizing and positioning.
// // CONCAT is needed because ##__LINE__ doesn't actually expand LINE
// #define UIWindow(ui, width, height)
//     UIBox CONCAT(_ui_container_, __LINE__) = MakeUIWindow((ui), (width), (height));
//     UIParent((ui), CONCAT(_ui_container_, __LINE__))

// static UIAction UIButtonEx(UIState* ui, Str8 text, UIFlag flags, UISize semantic_width, UISize semantic_height) {
//     UIBoxStyle style = {
//         .color_bg = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_COLOR),
//         .color_bg_hover = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_HOVER_COLOR),
//         .color_bg_down = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_DOWN_COLOR),
//         .color_bg_unclickable = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_UNCLICKABLE_COLOR),
//         .color_text = UIGetThemeColor(ui, UI_THEME_BUTTON_TEXT_COLOR),
//         .align =
//             {
//                 .x = UI_TEXT_ALIGN_X_CENTER,
//                 .y = UI_TEXT_ALIGN_Y_MIDDLE,
//             },
//     };

//     UIAction action = {0};
//     UIBoxEx(ui, text, flags | UI_FLAG_ABLE_TO_BE_CLICKED, semantic_width, semantic_height, style, &action);
//     return action;
// }

// LCORE UIAction UIButton(UIState* ui, Str8 text, UIFlag flags) {
//     return UIButtonEx(ui, text, flags, UISizePctParent(1.0f), UISizeText());
// }

// LCORE UIAction UILabel(UIState* ui, Str8 text, UIFlag flags) {
//     UIBoxStyle style = {
//         .color_bg = UIGetThemeColor(ui, UI_THEME_LABEL_BG_COLOR),
//         .color_text = UIGetThemeColor(ui, UI_THEME_LABEL_TEXT_COLOR),
//         .align =
//             {
//                 .x = UI_TEXT_ALIGN_X_LEFT,
//                 .y = UI_TEXT_ALIGN_Y_MIDDLE,
//             },
//     };

//     UIAction action = {0};
//     UIBoxEx(ui, text, flags & (~UI_FLAG_ABLE_TO_BE_CLICKED), UISizePctParent(1.0f), UISizeText(), style, &action);
//     return action;
// }

// static void UISpacer(UIState* ui, UISize semantic_width, UISize semantic_height) {
//     UIBoxStyle style = {0};
//     UIAction action = {0};
//     UIBoxEx(ui, Str8Zero(), UI_FLAG_HIDE, semantic_width, semantic_height, style, &action);
// }

// // Move the ui cursor to be at the previous line mark. Allows laying out horizontally.
// static void UISameLine(UIState* ui) {
//     UIBox* parent = UIGetParent(ui);
//     parent->cursor_top_left_offset = parent->cursor_top_left_offset_prev_line;
// }
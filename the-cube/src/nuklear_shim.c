#ifndef UNITY_BUILD
#include "raylib.h"

#endif

#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NUM_SEGMENTS 20

typedef struct nk_context NuklearCtx;
typedef struct nk_user_font NuklearFont;
typedef struct nk_color NuklearColor;
typedef struct nk_command NuklearCommand;

static Color RaylibColor2NuklearColor(NuklearColor color) {
    Color c = {color.r, color.g, color.b, color.a};
    return c;
}

static float NuklearCalcWidth(nk_handle handle, float height, const char *text, int len) {
    NK_UNUSED(handle);

    if (len > 0) {
        const char *subtext = TextSubtext(text, 0, len);
        return (float)MeasureText(subtext, (int)height) + height / 10.0f;
    }

    return 0;
}

static void NuklearCtxInputKeys(NuklearCtx *ctx) {}
static void NuklearCtxInputMouse(NuklearCtx *ctx) {
    Vector2 pos = GetMousePosition();
    nk_input_motion(ctx, pos.x, pos.y);

    nk_input_button(ctx, NK_BUTTON_LEFT, pos.x, pos.y, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
}

static void NuklearInit(NuklearCtx *ctx, NuklearFont *font) {
    font->height = 14;
    font->width = NuklearCalcWidth;
    font->userdata = nk_handle_ptr(NULL);
    int res = nk_init_default(ctx, font);
    assert(res);
}

static void NuklearUpdate(NuklearCtx *ctx, float delta_time) {
    // Update the time that has changed since last frame.
    ctx->delta_time_seconds = delta_time;

    // Update the input state.
    nk_input_begin(ctx);
    {
        NuklearCtxInputKeys(ctx);
        NuklearCtxInputMouse(ctx);
    }
    nk_input_end(ctx);

    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
        enum { EASY, HARD };
        static int op = EASY;
        static int property = 20;

        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button")) TraceLog(LOG_INFO, "button pressed!");
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
        nk_layout_row_dynamic(ctx, 22, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    }
    nk_end(ctx);
}

static void NuklearDraw(NuklearCtx *ctx) {
    const NuklearCommand *cmd;
    const float scale = 1.0f;

    nk_foreach(cmd, ctx) {
        switch (cmd->type) {
            case NK_COMMAND_NOP: {
                break;
            }

            case NK_COMMAND_SCISSOR: {
                // TODO(RobLoach): Verify if NK_COMMAND_SCISSOR works.
                const struct nk_command_scissor *s = (const struct nk_command_scissor *)cmd;
                BeginScissorMode((int)(s->x * scale), (int)(s->y * scale), (int)(s->w * scale), (int)(s->h * scale));
            } break;

            case NK_COMMAND_LINE: {
                const struct nk_command_line *l = (const struct nk_command_line *)cmd;
                Color color = RaylibColor2NuklearColor(l->color);
                Vector2 startPos = {(float)l->begin.x * scale, (float)l->begin.y * scale};
                Vector2 endPos = {(float)l->end.x * scale, (float)l->end.y * scale};
                DrawLineEx(startPos, endPos, l->line_thickness * scale, color);
            } break;

            case NK_COMMAND_CURVE: {
                const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
                Color color = RaylibColor2NuklearColor(q->color);
                // Vector2 start = {(float)q->begin.x, (float)q->begin.y};
                Vector2 start = {(float)q->begin.x * scale, (float)q->begin.y * scale};
                // Vector2 controlPoint1 = (Vector2){q->ctrl[0].x, q->ctrl[0].y};
                // Vector2 controlPoint2 = (Vector2){q->ctrl[1].x, q->ctrl[1].y};
                // Vector2 end = {(float)q->end.x, (float)q->end.y};
                Vector2 end = {(float)q->end.x * scale, (float)q->end.y * scale};
                // TODO: Encorporate segmented control point bezier curve?
                // DrawLineBezier(start, controlPoint1, (float)q->line_thickness, color);
                // DrawLineBezier(controlPoint1, controlPoint2, (float)q->line_thickness, color);
                // DrawLineBezier(controlPoint2, end, (float)q->line_thickness, color);
                // DrawLineBezier(start, end, (float)q->line_thickness, color);
                DrawLineBezier(start, end, (float)q->line_thickness * scale, color);
            } break;

            case NK_COMMAND_RECT: {
                const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
                Color color = RaylibColor2NuklearColor(r->color);
                Rectangle rect = {(float)r->x * scale, (float)r->y * scale, (float)r->w * scale, (float)r->h * scale};
                float roundness = (rect.width > rect.height) ? ((2 * r->rounding * scale) / rect.height)
                                                             : ((2 * r->rounding * scale) / rect.width);
                roundness = NK_CLAMP(0.0, roundness, 1.0);
                if (roundness > 0.0f) {
                    // DrawRectangleRoundedLines doesn't work in the same way as DrawRectangleLinesEx and it draws
                    // the outline outside the region defined by the rectangle. To compensate for that, shrink
                    // the rectangle by the thickness plus 1 (due to inconsistencies from DrawRectangleRoundedLines):
                    rect.x += ((float)r->line_thickness) * scale + 1.0;
                    rect.y += ((float)r->line_thickness) * scale + 1.0;
                    rect.width = NK_MAX(rect.width - (2 * ((float)r->line_thickness) * scale + 1.0), 0.0);
                    rect.height = NK_MAX(rect.height - (2 * ((float)r->line_thickness) * scale + 1.0), 0.0);
                    DrawRectangleRoundedLines(rect, roundness, NUM_SEGMENTS, (float)r->line_thickness * scale, color);
                } else {
                    DrawRectangleLinesEx(rect, r->line_thickness * scale, color);
                }
            } break;

            case NK_COMMAND_RECT_FILLED: {
                const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
                Color color = RaylibColor2NuklearColor(r->color);
                Rectangle rect = {(float)r->x * scale, (float)r->y * scale, (float)r->w * scale, (float)r->h * scale};
                float roundness = (rect.width > rect.height) ? ((2 * r->rounding * scale) / rect.height)
                                                             : ((2 * r->rounding * scale) / rect.width);
                roundness = NK_CLAMP(0.0, roundness, 1.0);
                if (roundness > 0.0f) {
                    DrawRectangleRounded(rect, roundness, NUM_SEGMENTS, color);
                } else {
                    DrawRectangleRec(rect, color);
                }
            } break;

            case NK_COMMAND_RECT_MULTI_COLOR: {
                const struct nk_command_rect_multi_color *rectangle = (const struct nk_command_rect_multi_color *)cmd;
                Rectangle position = {(float)rectangle->x * scale, (float)rectangle->y * scale,
                                      (float)rectangle->w * scale, (float)rectangle->h * scale};
                Color left = RaylibColor2NuklearColor(rectangle->left);
                Color top = RaylibColor2NuklearColor(rectangle->top);
                Color bottom = RaylibColor2NuklearColor(rectangle->bottom);
                Color right = RaylibColor2NuklearColor(rectangle->right);
                DrawRectangleGradientEx(position, left, bottom, right, top);
            } break;

            case NK_COMMAND_CIRCLE: {
                const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
                Color color = RaylibColor2NuklearColor(c->color);
                DrawEllipseLines((int)(c->x * scale + c->w * scale / 2.0f), (int)(c->y * scale + c->h * scale / 2.0f),
                                 (int)(c->w * scale / 2.0f), (int)(c->h * scale / 2.0f), color);
            } break;

            case NK_COMMAND_CIRCLE_FILLED: {
                const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
                Color color = RaylibColor2NuklearColor(c->color);
                DrawEllipse((int)(c->x * scale + c->w * scale / 2.0f), (int)(c->y * scale + c->h * scale / 2.0f),
                            (int)(c->w * scale / 2), (int)(c->h * scale / 2), color);
            } break;

            case NK_COMMAND_ARC: {
                const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;
                Color color = RaylibColor2NuklearColor(a->color);
                Vector2 center = {(float)a->cx, (float)a->cy};
                DrawRingLines(center, 0, a->r * scale, a->a[0] * RAD2DEG - 45, a->a[1] * RAD2DEG - 45, NUM_SEGMENTS,
                              color);
            } break;

            case NK_COMMAND_ARC_FILLED: {
                const struct nk_command_arc_filled *a = (const struct nk_command_arc_filled *)cmd;
                Color color = RaylibColor2NuklearColor(a->color);
                Vector2 center = {(float)a->cx * scale, (float)a->cy * scale};
                DrawRing(center, 0, a->r * scale, a->a[0] * RAD2DEG - 45, a->a[1] * RAD2DEG - 45, NUM_SEGMENTS, color);
            } break;

            case NK_COMMAND_TRIANGLE: {
                const struct nk_command_triangle *t = (const struct nk_command_triangle *)cmd;
                Color color = RaylibColor2NuklearColor(t->color);
                Vector2 point1 = {(float)t->b.x * scale, (float)t->b.y * scale};
                Vector2 point2 = {(float)t->a.x * scale, (float)t->a.y * scale};
                Vector2 point3 = {(float)t->c.x * scale, (float)t->c.y * scale};
                DrawTriangleLines(point1, point2, point3, color);
            } break;

            case NK_COMMAND_TRIANGLE_FILLED: {
                const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
                Color color = RaylibColor2NuklearColor(t->color);
                Vector2 point1 = {(float)t->b.x * scale, (float)t->b.y * scale};
                Vector2 point2 = {(float)t->a.x * scale, (float)t->a.y * scale};
                Vector2 point3 = {(float)t->c.x * scale, (float)t->c.y * scale};
                DrawTriangle(point1, point2, point3, color);
            } break;

            case NK_COMMAND_POLYGON: {
                // TODO: Confirm Polygon
                const struct nk_command_polygon *p = (const struct nk_command_polygon *)cmd;
                Color color = RaylibColor2NuklearColor(p->color);
                struct Vector2 *points = (struct Vector2 *)MemAlloc(p->point_count * (unsigned short)sizeof(Vector2));
                unsigned short i;
                for (i = 0; i < p->point_count; i++) {
                    points[i].x = p->points[i].x * scale;
                    points[i].y = p->points[i].y * scale;
                }
                DrawTriangleStrip(points, p->point_count, color);
                MemFree(points);
            } break;

            case NK_COMMAND_POLYGON_FILLED: {
                // TODO: Polygon filled expects counter clockwise order
                const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
                Color color = RaylibColor2NuklearColor(p->color);
                struct Vector2 *points = (struct Vector2 *)MemAlloc(p->point_count * (unsigned short)sizeof(Vector2));
                unsigned short i;
                for (i = 0; i < p->point_count; i++) {
                    points[i].x = p->points[i].x * scale;
                    points[i].y = p->points[i].y * scale;
                }
                DrawTriangleFan(points, p->point_count, color);
                MemFree(points);
            } break;

            case NK_COMMAND_POLYLINE: {
                // TODO: Polygon expects counter clockwise order
                const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
                Color color = RaylibColor2NuklearColor(p->color);
                struct Vector2 *points = (struct Vector2 *)MemAlloc(p->point_count * (unsigned short)sizeof(Vector2));
                unsigned short i;
                for (i = 0; i < p->point_count; i++) {
                    points[i].x = p->points[i].x * scale;
                    points[i].y = p->points[i].y * scale;
                }
                DrawTriangleStrip(points, p->point_count, color);
                MemFree(points);
            } break;

            case NK_COMMAND_TEXT: {
                const struct nk_command_text *text = (const struct nk_command_text *)cmd;
                Color color = RaylibColor2NuklearColor(text->foreground);
                float fontSize = text->font->height * scale;
                Font *font = (Font *)text->font->userdata.ptr;
                if (font != NULL) {
                    Vector2 position = {(float)text->x * scale, (float)text->y * scale};
                    DrawTextEx(*font, (const char *)text->string, position, fontSize, fontSize / 10.0f, color);
                } else {
                    DrawText((const char *)text->string, (int)(text->x * scale), (int)(text->y * scale), (int)fontSize,
                             color);
                }
            } break;

            case NK_COMMAND_IMAGE: {
                const struct nk_command_image *i = (const struct nk_command_image *)cmd;
                Texture texture = *(Texture *)i->img.handle.ptr;
                Rectangle source = {i->img.region[0], i->img.region[1], (float)i->img.region[2],
                                    (float)i->img.region[3]};
                Rectangle dest = {(float)i->x * scale, (float)i->y * scale, (float)i->w * scale, (float)i->h * scale};
                Vector2 origin = {0, 0};
                Color tint = RaylibColor2NuklearColor(i->col);
                DrawTexturePro(texture, source, dest, origin, 0, tint);
            } break;

            case NK_COMMAND_CUSTOM: {
                TraceLog(LOG_WARNING, "NUKLEAR: Unverified custom callback implementation NK_COMMAND_CUSTOM");
                const struct nk_command_custom *custom = (const struct nk_command_custom *)cmd;
                custom->callback(NULL, (short)(custom->x * scale), (short)(custom->y * scale),
                                 (unsigned short)(custom->w * scale), (unsigned short)(custom->h * scale),
                                 custom->callback_data);
            } break;

            default: {
                TraceLog(LOG_WARNING, "NUKLEAR: Missing implementation %i", cmd->type);
            } break;
        }
    }

    nk_clear(ctx);
}

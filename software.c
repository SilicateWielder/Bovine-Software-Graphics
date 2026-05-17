/*///////////////////////////////////////////////////////////////////////////*/
/* software.c - Bovine Software Graphics Driver                              */
/*///////////////////////////////////////////////////////////////////////////*/
/*                                                                           */
/* PURPOSE:                                                                  */
/*   Defines the abstract graphics driver ABI. Any graphics backend          */
/*   (software rasterizer, OpenGL, DirectX, etc.) implements this.           */
/*                                                                           */
/*///////////////////////////////////////////////////////////////////////////*/
/*                                                                           */
/* NOTICE:                                                                   */
/*   This file is written to strictly comply with C89 standards, no newer    */
/*   features are used.                                                      */
/*                                                                           */
/*   This file also compiles to a standalone .so, .dll or likewise, with no  */
/*   dependencies on other parts of the Bovine engine.                       */
/*                                                                           */
/*///////////////////////////////////////////////////////////////////////////*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphics_driver.h"

/*///////////////////////////////////////////////////////////////////////////*/
/* Context                                                                   */
/*///////////////////////////////////////////////////////////////////////////*/

struct BvRenderSurfaceContext {
    int width;
    int height;
    int *surface; /* RGBA pixels, row-major */
    int resizeable;
};

struct BvGraphicsContext {
    int width;
    int height;
    BvRenderSurfaceContext *framebuffer;
    int should_close;
};

typedef BvGraphicsContext BvScreenBufContext;

typedef struct {
    int red;
    int green;
    int blue;
    int alpha; 
} BvColor;

typedef struct {
    int x;
    int y;
} BvPoint2d;

typedef struct {
    BvPoint2d start;
    BvPoint2d end;
} BvPath2d;

/*///////////////////////////////////////////////////////////////////////////*/
/* Forward declarations (C89-safe, avoids implicit int/function conflicts)   */
/*///////////////////////////////////////////////////////////////////////////*/

static int bv_color_to_int(int red, int green, int blue, int alpha);
static BvColor bv_int_to_color(int color);
static BvColor bv_transparent_color_mix(BvColor src, BvColor dst);

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - init                                                   */
/*///////////////////////////////////////////////////////////////////////////*/

/* Initializes a surface instance and returns it. */
static BvRenderSurfaceContext *bv_render_surface_init(int width, int height, int resizeable) {
    BvRenderSurfaceContext *ctx;
    size_t pixel_count;
    size_t byte_count;

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "[Graphics] Invalid surface dimensions %d x %d\n", width, height);
        return NULL;
    }

    if (resizeable < 0 || resizeable > 1) {
        fprintf(stderr, "[Graphics] Invalid resizeable flag value %d, expected 0 or 1\n", resizeable);
        return NULL;
    }

    ctx = (BvRenderSurfaceContext *)malloc(sizeof(*ctx));
    if (ctx == NULL) {
        fprintf(stderr, "[Graphics] Failed to allocate memory for render surface context\n");
        return NULL;
    }

    /* Check if we have memory to allocate the surface. */
    if ((size_t)width > ((size_t)-1) / (size_t)height) {
        fprintf(stderr, "[Graphics] Requested surface dimensions are too large and may cause overflow\n");
        free(ctx);
        return NULL;
    }

    pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > ((size_t)-1) / sizeof(int)) {
        fprintf(stderr, "[Graphics] Surface byte size overflow\n");
        free(ctx);
        return NULL;
    }

    byte_count = pixel_count * sizeof(int);
    ctx->surface = (int *)malloc(byte_count);
    if (ctx->surface == NULL) {
        fprintf(stderr, "[Graphics] Failed to allocate memory for render surface\n");
        free(ctx);
        return NULL;
    }

    memset(ctx->surface, 0, byte_count); /* Clear surface to black (0x00000000) */

    ctx->width = width;
    ctx->height = height;
    ctx->resizeable = resizeable;

    fprintf(stderr, "[Graphics] Initialized render surface %d x %d, resizeable: %s\n", width, height, resizeable ? "yes" : "no");
    return ctx;
}

/* Initializes the screenbuffer using a render surface and proxies all surface-related operations to it. */
static BvGraphicsContext *bv_graphics_init(int width, int height) {
    BvScreenBufContext *ctx;
    BvRenderSurfaceContext *framebuffer;

    framebuffer = bv_render_surface_init(width, height, 1);
    if (framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Failed to initialize render surface for screen buffer\n");
        return NULL;
    }

    ctx = (BvScreenBufContext *)malloc(sizeof(*ctx));
    if (ctx == NULL) {
        fprintf(stderr, "[Graphics] Failed to allocate memory for graphics context\n");
        free(framebuffer->surface);
        free(framebuffer);
        return NULL;
    }

    ctx->width = width;
    ctx->height = height;
    ctx->framebuffer = framebuffer;
    ctx->should_close = 0;

    fprintf(stderr, "[Graphics] Initialized software graphics context with screen buffer %d x %d\n", width, height);
    return (BvGraphicsContext *)ctx;
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - getWidth                                               */
/*///////////////////////////////////////////////////////////////////////////*/
static int bv_render_surface_get_width(BvRenderSurfaceContext *surfctx) {
    int width_result;

    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot get surface width, render surface context is NULL.\n");
        return -1;
    }

    width_result = surfctx->width;
    return width_result;
}

/* Screenbuffer shim */
static int bv_graphics_get_width(BvGraphicsContext *bvctx) {
    int width_result;

    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot get graphics width, graphics context is NULL.\n");
        return -1;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Cannot get graphics width, framebuffer is NULL.\n");
        return -1;
    }

    width_result = bv_render_surface_get_width(((BvScreenBufContext *)bvctx)->framebuffer);
    return width_result;
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - getHeight                                              */
/*///////////////////////////////////////////////////////////////////////////*/

static int bg_render_surface_get_height(BvRenderSurfaceContext *surfctx) {
    int height_result;

    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot get surface height, render surface context is NULL.\n");
        return -1;
    }

    height_result = surfctx->height;
    return height_result;
}

/* Screenbuffer shim */
static int bv_graphics_get_height(BvGraphicsContext *bvctx) {
    int height_result;

    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot get graphics height, graphics context is NULL.\n");
        return -1;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Cannot get graphics height, framebuffer is NULL.\n");
        return -1;
    }

    height_result = bg_render_surface_get_height(((BvScreenBufContext *)bvctx)->framebuffer);
    return height_result;
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - shutdown                                               */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_graphics_shutdown(BvGraphicsContext *bvctx) {
    BvScreenBufContext *screen_ctx;

    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot shutdown graphics context, context is NULL.\n");
        return;
    }

    screen_ctx = (BvScreenBufContext *)bvctx;
    if (screen_ctx->framebuffer != NULL) {
        if (screen_ctx->framebuffer->surface != NULL) {
            free(screen_ctx->framebuffer->surface);
        } else {
            fprintf(stderr, "[Graphics] Warning: framebuffer surface is NULL during shutdown.\n");
        }
        free(screen_ctx->framebuffer);
    } else {
        fprintf(stderr, "[Graphics] Warning: framebuffer is NULL during shutdown.\n");
    }

    free(screen_ctx);
    fprintf(stderr, "[Graphics] Graphics context shutdown complete.\n");
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - begin_frame                                            */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_graphics_begin_frame(BvGraphicsContext *bvctx) {
    BvScreenBufContext *screen_ctx;
    size_t pixel_count;
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot begin frame, graphics context is NULL.\n");
        return;
    }

    screen_ctx = (BvScreenBufContext *)bvctx;
    /* Clear the framebuffer to black (0x00000000) at the start of each frame */
    if (screen_ctx->framebuffer != NULL && screen_ctx->framebuffer->surface != NULL) {
        pixel_count = (size_t)screen_ctx->framebuffer->width * (size_t)screen_ctx->framebuffer->height;
        memset(screen_ctx->framebuffer->surface, 0, pixel_count * sizeof(int));
    } else {
        fprintf(stderr, "[Graphics] Warning: framebuffer or surface is NULL during begin_frame.\n");
    }
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - end_frame                                              */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_graphics_end_frame(BvGraphicsContext *bvctx) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot end frame, graphics context is NULL.\n");
        return;
    }

    /* In a real graphics driver, this would present the framebuffer to the screen.
       For this software driver, we might just log that the frame has ended. */
    fprintf(stderr, "[Graphics] Frame ended.\n");
    (void)bvctx;
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - FlipScreen shim                                        */
/*///////////////////////////////////////////////////////////////////////////*/

static void bg_graphics_flip(BvGraphicsContext *bvctx) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot flip screen, graphics context is NULL.\n");
        return;
    }

    bv_graphics_end_frame(bvctx);
    bv_graphics_begin_frame(bvctx);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - should_close                                           */
/*///////////////////////////////////////////////////////////////////////////*/

static int bv_graphics_should_close(BvGraphicsContext *bvctx) {
    BvScreenBufContext *screen_ctx;

    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot check should_close, graphics context is NULL.\n");
        return 1; /* If context is NULL, we should close */
    }

    screen_ctx = (BvScreenBufContext *)bvctx;
    return screen_ctx->should_close;
}

/*///////////////////////////////////////////////////////////////////////////*/
/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - Begin Helpers                                          */
/*///////////////////////////////////////////////////////////////////////////*/
/*///////////////////////////////////////////////////////////////////////////*/

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_put_pixel_unchecked                  */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_put_pixel_unchecked(BvRenderSurfaceContext *surfctx, int x, int y, int color) {
    BvColor surfColor = bv_int_to_color(surfctx->surface[y * surfctx->width + x]);
    BvColor newColor = bv_transparent_color_mix(bv_int_to_color(color), surfColor);
    surfctx->surface[y * surfctx->width + x] = bv_color_to_int(newColor.red, newColor.green, newColor.blue, newColor.alpha);
}

static int bv_render_surface_get_pixel_unchecked(BvRenderSurfaceContext *surfctx, int x, int y) {
    return surfctx->surface[y * surfctx->width + x];
}

static int bv_render_surface_pixel_in_bounds(BvRenderSurfaceContext *surfctx, int x, int y) {
    return (x >= 0 && x < surfctx->width && y >= 0 && y < surfctx->height);
}

static int bv_color_to_int(int red, int green, int blue, int alpha) {
    return ((alpha & 0xFF) << 24) | 
           ((red   & 0xFF) << 16) | 
           ((green & 0xFF) << 8)  | 
           (blue & 0xFF);
}

static BvColor bv_int_to_color(int color) {
    BvColor result;

    result.alpha = (color >> 24) & 0xFF;
    result.red = (color >> 16) & 0xFF;
    result.green = (color >> 8) & 0xFF;
    result.blue = color & 0xFF;

    return result;
}

static int bv_soft_clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static double bv_soft_edge_func(double ax, double ay, double bx, double by, double cx, double cy) {
    double result = (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    return result;
}

/* Applies blending for non-opaque colors */
static BvColor bv_transparent_color_mix(BvColor src, BvColor dst) {
    BvColor result;

    if(src.alpha == 255) {
        return src; /* Fully opaque, no blending needed */
    } else if (src.alpha == 0) {
        return dst; /* Fully transparent, return destination color */
    }

    if (dst.alpha == 0) {
        return src; /* Fully transparent, return source color */
    }

    result.alpha = src.alpha + dst.alpha * (255 - src.alpha) / 255;
    result.red = (src.red * src.alpha + dst.red * dst.alpha * (255 - src.alpha) / 255) / result.alpha;
    result.green = (src.green * src.alpha + dst.green * dst.alpha * (255 - src.alpha) / 255) / result.alpha;
    result.blue = (src.blue * src.alpha + dst.blue * dst.alpha * (255 - src.alpha) / 255) / result.alpha;

    return result;
}

/*///////////////////////////////////////////////////////////////////////////*/
/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - End Helpers                                            */
/*///////////////////////////////////////////////////////////////////////////*/
/*///////////////////////////////////////////////////////////////////////////*/

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_get_color                            */
/*///////////////////////////////////////////////////////////////////////////*/

static int bv_render_surface_get_color(BvRenderSurfaceContext *surfctx, int x, int y) {
    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to get pixel color, surface context is NULL\n");
        return 0;
    }

    if (!bv_render_surface_pixel_in_bounds(surfctx, x, y)) {
        fprintf(stderr, "[Graphics] Attempted to get pixel color out of bounds at (%d, %d)\n", x, y);
        return 0; /* Return black for out-of-bounds */
    }
    return bv_render_surface_get_pixel_unchecked(surfctx, x, y);
}

/* Shim */

static int bv_graphics_get_color(BvGraphicsContext *bvctx, int x, int y) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to get pixel color, graphics context is NULL\n");
        return 0;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Attempted to get pixel color, framebuffer is NULL\n");
        return 0;
    }

    return bv_render_surface_get_color(((BvScreenBufContext *)bvctx)->framebuffer, x, y);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_blit_pixel                           */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_blit_pixel(BvRenderSurfaceContext *surfctx, int x, int y, int color) {
    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit pixel, surface context is NULL\n");
        return;
    }

    if (!bv_render_surface_pixel_in_bounds(surfctx, x, y)) {
        fprintf(stderr, "[Graphics] Attempted to blit pixel out of bounds at (%d, %d)\n", x, y);
        return; /* Ignore out-of-bounds blit */
    }

    bv_render_surface_put_pixel_unchecked(surfctx, x, y, color);
}

/* Shim */

static void bv_graphics_blit_pixel(BvGraphicsContext *bvctx, int x, int y, int color) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit pixel, graphics context is NULL\n");
        return;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit pixel, framebuffer is NULL\n");
        return;
    }

    bv_render_surface_blit_pixel(((BvScreenBufContext *)bvctx)->framebuffer, x, y, color);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_blit_line                            */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_blit_line(BvRenderSurfaceContext *surfctx, int x0, int y0, int x1, int y1, int color) {
    /* */

    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int err2;

    while (1) {
        if (bv_render_surface_pixel_in_bounds(surfctx, x0, y0)) {
            bv_render_surface_put_pixel_unchecked(surfctx, x0, y0, color);
        }
        if (x0 == x1 && y0 == y1) break;
        err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* Shim */

static void bv_graphics_blit_line(BvGraphicsContext *bvctx, int x0, int y0, int x1, int y1, int color) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit line, graphics context is NULL\n");
        return;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit line, framebuffer is NULL\n");
        return;
    }

    bv_render_surface_blit_line(((BvScreenBufContext *)bvctx)->framebuffer, x0, y0, x1, y1, color);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_blit_rectangle                       */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_blit_rectangle(BvRenderSurfaceContext *surfctx, int x, int y, int width, int height, int color) {
    int x_min;
    int y_min;
    int x_max;
    int y_max;
    int left_outside;
    int top_outside;
    int right_outside;
    int bottom_outside;

    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit rectangle, surface context is NULL\n");
        return;
    }

    if (width < 0) {
        x += width;
        width = -width;
    }

    if (height < 0) {
        y += height;
        height = -height;
    }

    if (width == 0 || height == 0) {
        return;
    }

    left_outside = x < 0;
    top_outside = y < 0;
    right_outside = x + width > surfctx->width;
    bottom_outside = y + height > surfctx->height;

    if (left_outside) {
        x_min = 0;
    } else {
        x_min = x;
    }

    if (top_outside) {
        y_min = 0;
    } else {
        y_min = y;
    }

    if (right_outside) {
        x_max = surfctx->width;
    } else {
        x_max = x + width;
    }

    if (bottom_outside) {
        y_max = surfctx->height;
    } else {
        y_max = y + height;
    }

    if (!left_outside) {
        bv_render_surface_blit_line(surfctx, x_min, y_min, x_min, y_max - 1, color);
    }

    if (!top_outside) {
        bv_render_surface_blit_line(surfctx, x_min, y_min, x_max - 1, y_min, color);
    }

    if (!right_outside) {
        bv_render_surface_blit_line(surfctx, x_max - 1, y_min, x_max - 1, y_max - 1, color);
    }

    if (!bottom_outside) {
        bv_render_surface_blit_line(surfctx, x_min, y_max - 1, x_max - 1, y_max - 1, color);
    }
}

/* Shim */

static void bv_graphics_blit_rectangle(BvGraphicsContext *bvctx, int x, int y, int width, int height, int color) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit rectangle, graphics context is NULL\n");
        return;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit rectangle, framebuffer is NULL\n");
        return;
    }

    bv_render_surface_blit_rectangle(((BvScreenBufContext *)bvctx)->framebuffer, x, y, width, height, color);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_blit_filled_rectangle                */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_blit_filled_rectangle(BvRenderSurfaceContext *surfctx, int x, int y, int width, int height, int color) {
    int x_min;
    int y_min;
    int x_max;
    int y_max;
    int i;
    int j;
    
    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit filled rectangle, surface context is NULL\n");
        return;
    }

    if (width < 0) {
        x += width;
        width = -width;
    }

    if (height < 0) {
        y += height;
        height = -height;
    }

    if (width == 0 || height == 0) {
        return; /* Nothing to blit */
    }

    if (x < 0) {
        x_min = 0;
    } else {
        x_min = x;
    }

    if (y < 0) {
        y_min = 0;
    } else {
        y_min = y;
    }

    if (x + width > surfctx->width) {
        x_max = surfctx->width;
    } else {
        x_max = x + width;
    }

    if (y + height > surfctx->height) {
        y_max = surfctx->height;
    } else {
        y_max = y + height;
    }

    for (j = y_min; j < y_max; j++) {
        for (i = x_min; i < x_max; i++) {
            bv_render_surface_put_pixel_unchecked(surfctx, i, j, color);
        }
    }
}

/* Shim */

static void bv_graphics_blit_filled_rectangle(BvGraphicsContext *bvctx, int x, int y, int width, int height, int color) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit filled rectangle, graphics context is NULL\n");
        return;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Attempted to blit filled rectangle, framebuffer is NULL\n");
        return;
    }

    bv_render_surface_blit_filled_rectangle(((BvScreenBufContext *)bvctx)->framebuffer, x, y, width, height, color);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_blit_textured_tri                    */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_blit_textured_tri (
    BvRenderSurfaceContext *dst, 
    BvRenderSurfaceContext *src, 
    int x1, int y1, double u1, double v1, 
    int x2, int y2, double u2, double v2, 
    int x3, int y3, double u3, double v3
) {
    int min_x, min_y, max_x, max_y;
    int px, py;
    double area;
    double pxf, pyf;

    double w0, w1, w2;
    double uu, vv;
    int sx, sy, src_color;

    if (src == NULL || dst == NULL || src->surface == NULL || dst->surface == NULL) {
        fprintf(stderr, "[Graphics] Cannot blit textured triangle, source or destination surface context is NULL.\n");
        return;
    }

    if (src->width <= 0 || src->height <= 0 || dst->width <= 0 || dst->height <= 0) {
        fprintf(stderr, "[Graphics] Cannot blit textured triangle, source or destination surface has invalid dimensions.\n");
        return;
    }

    min_x = x1;
    if (x2 < min_x) min_x = x2;
    if (x3 < min_x) min_x = x3;

    min_y = y1;
    if (y2 < min_y) min_y = y2;
    if (y3 < min_y) min_y = y3;

    max_x = x1;
    if (x2 > max_x) max_x = x2;
    if (x3 > max_x) max_x = x3;

    max_y = y1;
    if (y2 > max_y) max_y = y2;
    if (y3 > max_y) max_y = y3;

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x > dst->width) max_x = dst->width;
    if (max_y > dst->height) max_y = dst->height;

    if (min_x >= max_x || min_y >= max_y) {
        return; /* No pixels to draw */
    }

    area = bv_soft_edge_func((double)x1, (double)y1, (double)x2, (double)y2, (double)x3, (double)y3);
    if (area == 0.0) {
        return; /* Degenerate triangle */
    }

    for (py = min_y; py < max_y; py++) {
        for (px = min_x; px < max_x; px++) {
            pxf = (double)px + 0.5;
            pyf = (double)py + 0.5;

            w0 = bv_soft_edge_func((double)x2, (double)y2, (double)x3, (double)y3, pxf, pyf);
            w1 = bv_soft_edge_func((double)x3, (double)y3, (double)x1, (double)y1, pxf, pyf);
            w2 = bv_soft_edge_func((double)x1, (double)y1, (double)x2, (double)y2, pxf, pyf);

            if ((area > 0.0 && (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0)) ||
                (area < 0.0 && (w0 <= 0.0 && w1 <= 0.0 && w2 <= 0.0))) {
                w0 /= area;
                w1 /= area;
                w2 /= area;

                uu = w0 * u1 + w1 * u2 + w2 * u3;
                vv = w0 * v1 + w1 * v2 + w2 * v3;

                sx = (int)(uu + 0.5);
                sy = (int)(vv + 0.5);

                sx = bv_soft_clamp(sx, 0, src->width - 1);
                sy = bv_soft_clamp(sy, 0, src->height - 1);

                src_color = bv_render_surface_get_pixel_unchecked(src, sx, sy);
                bv_render_surface_put_pixel_unchecked(dst, px, py, src_color);
            }
        }
    }
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_transform_blit                       */
/*///////////////////////////////////////////////////////////////////////////*/
/* surfctx, source_surface, x1, y1, x2, y2, x3, y3, x4, y4 */

static void bv_render_surface_transform_blit(
    BvRenderSurfaceContext *surfctx, 
    BvRenderSurfaceContext *sourcectx, 
    int x1, int y1, int x2, int y2,
    int x3, int y3, int x4, int y4
) {
    /* For simplicity, this function is not implemented in the software driver. */
    double u0, v0, u1, v1, u2, v2, u3, v3;

    if (surfctx == NULL || sourcectx == NULL) {
        fprintf(stderr, "[Graphics] Cannot perform transform blit, source or destination surface context is NULL.\n");
        return;
    }

    if (sourcectx-> width <=0 || sourcectx->height <= 0) {
        fprintf(stderr, "[Graphics] Cannot perform transform blit, source surface has invalid dimensions.\n");
        return;
    }

    /* Map out uv positions */
    u0 = 0.0;                            v0 = 0.0;
    u1 = (double)(sourcectx->width - 1); v1 = 0.0;
    u2 = (double)(sourcectx->width - 1); v2 = (double)(sourcectx->height - 1);
    u3 = 0.0;                            v3 = (double)(sourcectx->height - 1);

    bv_render_surface_blit_textured_tri(
        surfctx, sourcectx, 
        x1, y1, u0, v0, 
        x2, y2, u1, v1, 
        x3, y3, u2, v2
    );

    bv_render_surface_blit_textured_tri(
        surfctx, sourcectx, 
        x1, y1, u0, v0, 
        x3, y3, u2, v2, 
        x4, y4, u3, v3
    );
}

/* Shim */

static void bv_graphics_transform_blit(
    BvGraphicsContext *bvctx, 
    BvRenderSurfaceContext *sourcectx, 
    int x1, int y1, int x2, int y2,
    int x3, int y3, int x4, int y4
) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot perform transform blit, graphics context is NULL.\n");
        return;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Cannot perform transform blit, framebuffer is NULL.\n");
        return;
    }

    bv_render_surface_transform_blit(((BvScreenBufContext *)bvctx)->framebuffer, sourcectx, x1, y1, x2, y2, x3, y3, x4, y4);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_apply_color_mask                     */
/*///////////////////////////////////////////////////////////////////////////*/

static void bv_render_surface_apply_color_mask(BvRenderSurfaceContext *surfctx, int color_mask) {
    int x, y;
    int pixel_color;
    BvColor pixel_bvcolor;
    BvColor mask_bvcolor;

    if (surfctx == NULL || surfctx->surface == NULL) {
        fprintf(stderr, "[Graphics] Cannot apply color mask, surface context is NULL\n");
        return;
    }

    mask_bvcolor = bv_int_to_color(color_mask);

    for (y = 0; y < surfctx->height; y++) {
        for (x = 0; x < surfctx->width; x++) {
            pixel_color = bv_render_surface_get_pixel_unchecked(surfctx, x, y);
            pixel_bvcolor = bv_int_to_color(pixel_color);

            /* Apply the color mask using alpha blending */
            pixel_bvcolor.red = (pixel_bvcolor.red * mask_bvcolor.red) / 255;
            pixel_bvcolor.green = (pixel_bvcolor.green * mask_bvcolor.green) / 255;
            pixel_bvcolor.blue = (pixel_bvcolor.blue * mask_bvcolor.blue) / 255;
            pixel_bvcolor.alpha = (pixel_bvcolor.alpha * mask_bvcolor.alpha) / 255;

            bv_render_surface_put_pixel_unchecked(
                surfctx,
                x,
                y,
                bv_color_to_int(
                    pixel_bvcolor.red,
                    pixel_bvcolor.green,
                    pixel_bvcolor.blue,
                    pixel_bvcolor.alpha
                )
            );
        }
    }
}

/* Shim */

static void bv_graphics_apply_color_mask(BvGraphicsContext *bvctx, int color_mask) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot apply color mask, graphics context is NULL\n");
        return;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Cannot apply color mask, framebuffer is NULL\n");
        return;
    }

    bv_render_surface_apply_color_mask(((BvScreenBufContext *)bvctx)->framebuffer, color_mask);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* BvGraphicsDriver - bv_render_surface_get_buffer                           */
/*///////////////////////////////////////////////////////////////////////////*/

static int *bv_render_surface_get_framebuffer(BvRenderSurfaceContext *surfctx) {
    if (surfctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot get framebuffer, surface context is NULL\n");
        return NULL;
    }

    return surfctx->surface;
}

/* Shim */

static int *bv_graphics_get_framebuffer(BvGraphicsContext *bvctx) {
    if (bvctx == NULL) {
        fprintf(stderr, "[Graphics] Cannot get framebuffer, graphics context is NULL\n");
        return NULL;
    }

    if (((BvScreenBufContext *)bvctx)->framebuffer == NULL) {
        fprintf(stderr, "[Graphics] Cannot get framebuffer, framebuffer is NULL\n");
        return NULL;
    }

    return bv_render_surface_get_framebuffer(((BvScreenBufContext *)bvctx)->framebuffer);
}

/*///////////////////////////////////////////////////////////////////////////*/
/* Driver Descriptor                                                         */
/*///////////////////////////////////////////////////////////////////////////*/

static BvGraphicsDriver bv_graphics_driver = {
    "software", /* Name for the driver */
    "0.1",      /* Version string for the driver */
    "null",     /* Api requirements string, useful for signaling to the engine what host capabilities we need. */
    bv_graphics_init,
    bv_graphics_shutdown,
    bv_graphics_begin_frame,
    bv_graphics_end_frame,
    bv_graphics_should_close,
    bg_graphics_flip,
    bv_graphics_get_width,
    bv_graphics_get_height,
    bv_graphics_blit_pixel,
    bv_graphics_blit_line,
    bv_graphics_blit_rectangle,
    bv_graphics_blit_filled_rectangle,
    bv_graphics_get_color,
    bv_graphics_apply_color_mask,
    bv_graphics_get_framebuffer,
    bv_graphics_transform_blit,
    bv_render_surface_get_width,
    bv_render_surface_get_color,
    bv_render_surface_blit_pixel,
    bv_render_surface_blit_line,
    bv_render_surface_blit_rectangle,
    bv_render_surface_blit_filled_rectangle,
    bv_render_surface_get_framebuffer
};

BvGraphicsDriver *get_bv_graphics_driver(void) {
    return &bv_graphics_driver; 
}
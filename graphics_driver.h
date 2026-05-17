/*///////////////////////////////////////////////////////////////////////////*/
/* graphics_driver.h - Bovine Graphics Driver Interface                     */
/*///////////////////////////////////////////////////////////////////////////*/
/*                                                                           */
/* PURPOSE:                                                                  */
/*   Defines the abstract graphics driver ABI. Any graphics backend          */
/*   (software rasterizer, OpenGL, DirectX, etc.) implements this.           */
/*                                                                           */
/*///////////////////////////////////////////////////////////////////////////*/
/*                                                                           */
/* NOTICE:                                                                   */
/*   engine.c uses only these types and does not include backend-specific    */
/*   headers.                                                                */
/*                                                                           */
/*///////////////////////////////////////////////////////////////////////////*/

#ifndef BV_GRAPHICS_DRIVER_H
#define BV_GRAPHICS_DRIVER_H

typedef struct BvGraphicsContext BvGraphicsContext;
typedef struct BvRenderSurfaceContext BvRenderSurfaceContext;

typedef struct BvGraphicsDriver {
	const char *name;
	const char *version;
	const char *api_requirements;

	BvGraphicsContext *(*init)(int width, int height);
	void (*shutdown)(BvGraphicsContext *ctx);
	void (*begin_frame)(BvGraphicsContext *ctx);
	void (*end_frame)(BvGraphicsContext *ctx);
	int (*should_close)(BvGraphicsContext *ctx);
	void (*flip_screen)(BvGraphicsContext *ctx);

	int (*get_width)(BvGraphicsContext *ctx);
	int (*get_height)(BvGraphicsContext *ctx);
	void (*blit_pixel)(BvGraphicsContext *ctx, int x, int y, int color);
	void (*blit_line)(BvGraphicsContext *ctx, int x0, int y0, int x1, int y1, int color);
	void (*blit_rectangle)(BvGraphicsContext *ctx, int x, int y, int width, int height, int color);
	void (*blit_filled_rectangle)(BvGraphicsContext *ctx, int x, int y, int width, int height, int color);
	int (*get_color)(BvGraphicsContext *ctx, int x, int y);
	void (*apply_color_mask)(BvGraphicsContext *ctx, int color_mask);
	int *(*get_framebuffer)(BvGraphicsContext *ctx);
	void (*transform_blit)(
		BvGraphicsContext *ctx,
		BvRenderSurfaceContext *source,
		int x1, int y1,
		int x2, int y2,
		int x3, int y3,
		int x4, int y4
	);

	int (*surface_get_width)(BvRenderSurfaceContext *surface);
	int (*surface_get_color)(BvRenderSurfaceContext *surface, int x, int y);
	void (*surface_blit_pixel)(BvRenderSurfaceContext *surface, int x, int y, int color);
	void (*surface_blit_line)(BvRenderSurfaceContext *surface, int x0, int y0, int x1, int y1, int color);
	void (*surface_blit_rectangle)(BvRenderSurfaceContext *surface, int x, int y, int width, int height, int color);
	void (*surface_blit_filled_rectangle)(BvRenderSurfaceContext *surface, int x, int y, int width, int height, int color);
	int *(*surface_get_framebuffer)(BvRenderSurfaceContext *surface);
} BvGraphicsDriver;

BvGraphicsDriver *get_bv_graphics_driver(void);

#endif /* BV_GRAPHICS_DRIVER_H */


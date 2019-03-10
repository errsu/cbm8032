//
// libshapes: high-level OpenVG API
// Anthony Starks (ajstarks@gmail.com)
//
// Additional outline / windowing functions
// Paeryn (github.com/paeryn)
//
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <assert.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"
#include "bcm_host.h"

typedef struct {
	// Screen dimensions
	uint32_t screen_width;
	uint32_t screen_height;
	// dispman window
	DISPMANX_ELEMENT_HANDLE_T element;

	// EGL data
	EGLDisplay display;

	EGLSurface surface;
	EGLContext context;
} STATE_T;

static STATE_T _state, *state = &_state;	// global graphics state

// oglinit sets the display, OpenVGL context and screen information
// state holds the display information
void oglinit() {

	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	static VC_DISPMANX_ALPHA_T alpha = {
		DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
		255, 0
	};

	static const EGLint attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	EGLConfig config;

	memset(state, 0, sizeof(*state));

	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(state->display != EGL_NO_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	assert(EGL_FALSE != result);

	// bind OpenVG API
	eglBindAPI(EGL_OPENVG_API);

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
	assert(state->context != EGL_NO_CONTEXT);

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */ , &state->screen_width,
					    &state->screen_height);
	assert(success >= 0);

	vc_dispmanx_rect_set(&dst_rect, 0, 0, state->screen_width, state->screen_height);
	vc_dispmanx_rect_set(&src_rect, 0, 0, state->screen_width << 16, state->screen_height << 16);

	dispman_display = vc_dispmanx_display_open(0 /* LCD */ );
	dispman_update = vc_dispmanx_update_start(0);

	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0 /*layer */ , &dst_rect, 0 /*src */ ,
						  &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 0 /*clamp */ ,
						  0 /*transform */ );

	state->element = dispman_element;
	nativewindow.element = dispman_element;
	nativewindow.width = state->screen_width;
	nativewindow.height = state->screen_height;
	vc_dispmanx_update_submit_sync(dispman_update);

	state->surface = eglCreateWindowSurface(state->display, config, &nativewindow, NULL);
	assert(state->surface != EGL_NO_SURFACE);

	// preserve the buffers on swap
	result = eglSurfaceAttrib(state->display, state->surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
	assert(EGL_FALSE != result);

	// connect the context to the surface
	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(EGL_FALSE != result);
}

//
// Terminal settings
//

// terminal settings structures
struct termios new_term_attr;
struct termios orig_term_attr;

// SaveTerm saves the current terminal settings
void SaveTerm() {
	tcgetattr(fileno(stdin), &orig_term_attr);
}

// RawTerm sets the terminal to raw mode
void RawTerm() {
	memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
	new_term_attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	new_term_attr.c_cc[VTIME] = 0;
	new_term_attr.c_cc[VMIN] = 0;
	tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
}

// restore resets the terminal to the previously saved setting
void RestoreTerm() {
	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
}

// MakeImage makes an image from a raw raster of red, green, blue, alpha values
void MakeImage(VGfloat x, VGfloat y, int w, int h, VGubyte * data) {
	unsigned int dstride = w * 4;
	VGImageFormat rgbaFormat = VG_sABGR_8888;
	VGImage img = vgCreateImage(rgbaFormat, w, h, VG_IMAGE_QUALITY_BETTER);
	vgImageSubData(img, (void *)data, dstride, rgbaFormat, 0, 0, w, h);
	vgSetPixels(x, y, img, 0, 0, w, h);
	vgDestroyImage(img);
}

// dumpscreen writes the raster
void dumpscreen(int w, int h, FILE * fp) {
	void *ScreenBuffer = malloc(w * h * 4);
	vgReadPixels(ScreenBuffer, (w * 4), VG_sABGR_8888, 0, 0, w, h);
	fwrite(ScreenBuffer, 1, w * h * 4, fp);
	free(ScreenBuffer);
}

// InitOpenVG sets the system to its initial state
void InitOpenVG(int *w, int *h) {
	bcm_host_init();
	oglinit();
	*w = state->screen_width;
	*h = state->screen_height;
}

// FinishOpenVG cleans up
void FinishOpenVG() {
	eglSwapBuffers(state->display, state->surface);
	eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(state->display, state->surface);
	eglDestroyContext(state->display, state->context);
	eglTerminate(state->display);
}

//
// Transformations
//

// Translate the coordinate system to x,y
void Translate(VGfloat x, VGfloat y) {
	vgTranslate(x, y);
}

// Rotate around angle r
void Rotate(VGfloat r) {
	vgRotate(r);
}

// Shear shears the x coordinate by x degrees, the y coordinate by y degrees
void Shear(VGfloat x, VGfloat y) {
	vgShear(x, y);
}

// Scale scales by  x, y
void Scale(VGfloat x, VGfloat y) {
	vgScale(x, y);
}

//
// Style functions
//

// SetFill sets the fill color
void SetFill(VGfloat color[4]) {
	VGPaint fillPaint = vgCreatePaint();
	vgSetParameteri(fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(fillPaint, VG_PAINT_COLOR, 4, color);
	vgSetPaint(fillPaint, VG_FILL_PATH);
	vgDestroyPaint(fillPaint);
}

// SetStroke sets the stroke color
void SetStroke(VGfloat color[4]) {
	VGPaint strokePaint = vgCreatePaint();
	vgSetParameteri(strokePaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(strokePaint, VG_PAINT_COLOR, 4, color);
	vgSetPaint(strokePaint, VG_STROKE_PATH);
	vgDestroyPaint(strokePaint);
}

// StrokeWidth sets the stroke width
void StrokeWidth(VGfloat width) {
	vgSetf(VG_STROKE_LINE_WIDTH, width);
	vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
	vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
}

//
// Color functions
//
//

// RGBA fills a color vectors from a RGBA quad.
void RGBA(unsigned int r, unsigned int g, unsigned int b, VGfloat a, VGfloat color[4]) {
	if (r > 255) {
		r = 0;
	}
	if (g > 255) {
		g = 0;
	}
	if (b > 255) {
		b = 0;
	}
	if (a < 0.0 || a > 1.0) {
		a = 1.0;
	}
	color[0] = (VGfloat) r / 255.0f;
	color[1] = (VGfloat) g / 255.0f;
	color[2] = (VGfloat) b / 255.0f;
	color[3] = a;
}

// RGB returns a solid color from a RGB triple
void RGB(unsigned int r, unsigned int g, unsigned int b, VGfloat color[4]) {
	RGBA(r, g, b, 1.0f, color);
}

// Stroke sets the stroke color, defined as a RGB triple.
void Stroke(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat color[4];
	RGBA(r, g, b, a, color);
	SetStroke(color);
}

// Fill sets the fillcolor, defined as a RGBA quad.
void Fill(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat color[4];
	RGBA(r, g, b, a, color);
	SetFill(color);
}

// setstops sets color stops for gradients
void setstop(VGPaint paint, VGfloat * stops, int n) {
	VGboolean multmode = VG_FALSE;
	VGColorRampSpreadMode spreadmode = VG_COLOR_RAMP_SPREAD_REPEAT;
	vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, spreadmode);
	vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_PREMULTIPLIED, multmode);
	vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, 5 * n, stops);
	vgSetPaint(paint, VG_FILL_PATH);
}

// LinearGradient fills with a linear gradient
void FillLinearGradient(VGfloat x1, VGfloat y1, VGfloat x2, VGfloat y2, VGfloat * stops, int ns) {
	VGfloat lgcoord[4] = { x1, y1, x2, y2 };
	VGPaint paint = vgCreatePaint();
	vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
	vgSetParameterfv(paint, VG_PAINT_LINEAR_GRADIENT, 4, lgcoord);
	setstop(paint, stops, ns);
	vgDestroyPaint(paint);
}

// RadialGradient fills with a linear gradient
void FillRadialGradient(VGfloat cx, VGfloat cy, VGfloat fx, VGfloat fy, VGfloat radius, VGfloat * stops, int ns) {
	VGfloat radialcoord[5] = { cx, cy, fx, fy, radius };
	VGPaint paint = vgCreatePaint();
	vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_GRADIENT);
	vgSetParameterfv(paint, VG_PAINT_RADIAL_GRADIENT, 5, radialcoord);
	setstop(paint, stops, ns);
	vgDestroyPaint(paint);
}

// ClipRect limits the drawing area to specified rectangle
void ClipRect(VGint x, VGint y, VGint w, VGint h) {
	vgSeti(VG_SCISSORING, VG_TRUE);
	VGint coords[4] = { x, y, w, h };
	vgSetiv(VG_SCISSOR_RECTS, 4, coords);
}

// ClipEnd stops limiting drawing area to specified rectangle
void ClipEnd() {
	vgSeti(VG_SCISSORING, VG_FALSE);
}

// newpath creates path data
// Changed capabilities as others not needed at the moment - allows possible
// driver optimisations.
VGPath newpath() {
	return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_APPEND_TO);	// Other capabilities not needed
}

// makecurve makes path data using specified segments and coordinates
void makecurve(VGubyte * segments, VGfloat * coords, VGbitfield flags) {
	VGPath path = newpath();
	vgAppendPathData(path, 2, segments, coords);
	vgDrawPath(path, flags);
	vgDestroyPath(path);
}

// CBezier makes a quadratic bezier curve
void Cbezier(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat px, VGfloat py, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_CUBIC_TO };
	VGfloat coords[] = { sx, sy, cx, cy, px, py, ex, ey };
	makecurve(segments, coords, VG_FILL_PATH | VG_STROKE_PATH);
}

// QBezier makes a quadratic bezier curve
void Qbezier(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_QUAD_TO };
	VGfloat coords[] = { sx, sy, cx, cy, ex, ey };
	makecurve(segments, coords, VG_FILL_PATH | VG_STROKE_PATH);
}

// interleave interleaves arrays of x, y into a single array
void interleave(VGfloat * x, VGfloat * y, int n, VGfloat * points) {
	while (n--) {
		*points++ = *x++;
		*points++ = *y++;
	}
}

// poly makes either a polygon or polyline
void poly(VGfloat * x, VGfloat * y, VGint n, VGbitfield flag) {
	VGfloat points[n * 2];
	VGPath path = newpath();
	interleave(x, y, n, points);
	vguPolygon(path, points, n, VG_FALSE);
	vgDrawPath(path, flag);
	vgDestroyPath(path);
}

// Polygon makes a filled polygon with vertices in x, y arrays
void Polygon(VGfloat * x, VGfloat * y, VGint n) {
	poly(x, y, n, VG_FILL_PATH);
}

// Polyline makes a polyline with vertices at x, y arrays
void Polyline(VGfloat * x, VGfloat * y, VGint n) {
	poly(x, y, n, VG_STROKE_PATH);
}

// Rect makes a rectangle at the specified location and dimensions
void Rect(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguRect(path, x, y, w, h);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Line makes a line from (x1,y1) to (x2,y2)
void Line(VGfloat x1, VGfloat y1, VGfloat x2, VGfloat y2) {
	VGPath path = newpath();
	vguLine(path, x1, y1, x2, y2);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Roundrect makes an rounded rectangle at the specified location and dimensions
void Roundrect(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat rw, VGfloat rh) {
	VGPath path = newpath();
	vguRoundRect(path, x, y, w, h, rw, rh);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Ellipse makes an ellipse at the specified location and dimensions
void Ellipse(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguEllipse(path, x, y, w, h);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Circle makes a circle at the specified location and dimensions
void Circle(VGfloat x, VGfloat y, VGfloat r) {
	Ellipse(x, y, r, r);
}

// Arc makes an elliptical arc at the specified location and dimensions
void Arc(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat sa, VGfloat aext) {
	VGPath path = newpath();
	vguArc(path, x, y, w, h, sa, aext, VGU_ARC_OPEN);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Start begins the picture, clearing a rectangular region with a specified color
void Start(int width, int height) {
	VGfloat color[4] = { 1, 1, 1, 1 };
	vgSetfv(VG_CLEAR_COLOR, 4, color);
	vgClear(0, 0, width, height);
	color[0] = 0, color[1] = 0, color[2] = 0;
	SetFill(color);
	SetStroke(color);
	StrokeWidth(0);
	vgLoadIdentity();
}

// End checks for errors, and renders to the display
void End() {
	assert(vgGetError() == VG_NO_ERROR);
	eglSwapBuffers(state->display, state->surface);
	assert(eglGetError() == EGL_SUCCESS);
}

// SaveEnd dumps the raster before rendering to the display
void SaveEnd(const char *filename) {
	FILE *fp;
	assert(vgGetError() == VG_NO_ERROR);
	if (strlen(filename) == 0) {
		dumpscreen(state->screen_width, state->screen_height, stdout);
	} else {
		fp = fopen(filename, "wb");
		if (fp != NULL) {
			dumpscreen(state->screen_width, state->screen_height, fp);
			fclose(fp);
		}
	}
	eglSwapBuffers(state->display, state->surface);
	assert(eglGetError() == EGL_SUCCESS);
}

// Backgroud clears the screen to a solid background color
void Background(unsigned int r, unsigned int g, unsigned int b) {
	VGfloat colour[4];
	RGB(r, g, b, colour);
	vgSetfv(VG_CLEAR_COLOR, 4, colour);
	vgClear(0, 0, state->screen_width, state->screen_height);
}

// BackgroundRGB clears the screen to a background color with alpha
void BackgroundRGB(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat colour[4];
	RGBA(r, g, b, a, colour);
	vgSetfv(VG_CLEAR_COLOR, 4, colour);
	vgClear(0, 0, state->screen_width, state->screen_height);
}

// WindowClear clears the window to previously set background colour
void WindowClear() {
	vgClear(0, 0, state->screen_width, state->screen_height);
}

// AreaClear clears a given rectangle in window coordinates (not affected by
// transformations)
void AreaClear(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	vgClear(x, y, w, h);
}

// Outlined shapes
// Hollow shapes -because filling still happens even with a fill of 0,0,0,0
// unlike where using a strokewidth of 0 disables the stroke.
// Either this or change the original functions to require the VG_x_PATH flags

// CBezier makes a quadratic bezier curve, stroked
void CbezierOutline(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat px, VGfloat py, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_CUBIC_TO };
	VGfloat coords[] = { sx, sy, cx, cy, px, py, ex, ey };
	makecurve(segments, coords, VG_STROKE_PATH);
}

// QBezierOutline makes a quadratic bezier curve, outlined
void QbezierOutline(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_QUAD_TO };
	VGfloat coords[] = { sx, sy, cx, cy, ex, ey };
	makecurve(segments, coords, VG_STROKE_PATH);
}

// RectOutline makes a rectangle at the specified location and dimensions, outlined
void RectOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguRect(path, x, y, w, h);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// RoundrectOutline  makes an rounded rectangle at the specified location and dimensions, outlined
void RoundrectOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat rw, VGfloat rh) {
	VGPath path = newpath();
	vguRoundRect(path, x, y, w, h, rw, rh);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// EllipseOutline makes an ellipse at the specified location and dimensions, outlined
void EllipseOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguEllipse(path, x, y, w, h);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// CircleOutline makes a circle at the specified location and dimensions, outlined
void CircleOutline(VGfloat x, VGfloat y, VGfloat r) {
	EllipseOutline(x, y, r, r);
}

// ArcOutline makes an elliptical arc at the specified location and dimensions, outlined
void ArcOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat sa, VGfloat aext) {
	VGPath path = newpath();
	vguArc(path, x, y, w, h, sa, aext, VGU_ARC_OPEN);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

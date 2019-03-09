//
// shapedemo: testbed for OpenVG APIs
// Anthony Starks (ajstarks@gmail.com)
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "shapes.h"

void imagetest(int w, int h) {
	int imgw = 422, imgh = 238;
	VGfloat cx = (w / 2) - (imgw / 2), cy = (h / 2) - (imgh / 2);
	Start(w, h);
	Background(0, 0, 0);
	VGImage img = createImageFromJpeg("desert1.jpg");
	vgSetPixels(cx, cy, img, 0, 0, imgw, imgh);
	vgDestroyImage(img);
	End();
}

// wait for a specific character 
void waituntil(int endchar) {
    int key;

    for (;;) {
        key = getchar();
        if (key == endchar || key == '\n') {
            break;
        }
    }
}

// main initializes the system and shows the picture. 
// Exit and clean up when you hit [RETURN].
int main(int argc, char **argv) {
	int w, h;
	saveterm();
	init(&w, &h);
	rawterm();
	imagetest(w, h);
	waituntil(0x1b);
	restoreterm();
	finish();
	return 0;
}

/////////////////////////////////////////////////////////////////////
#if 0
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
#include <jpeglib.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"
#include "bcm_host.h"
#include "eglstate.h"  // data structures for graphics state

static STATE_T _state, *state = &_state;	// global graphics state
static int init_x = 0;		// Initial window position and size
static int init_y = 0;
static unsigned int init_w = 0;
static unsigned int init_h = 0;
//
// Terminal settings
//

// terminal settings structures
struct termios new_term_attr;
struct termios orig_term_attr;

// saveterm saves the current terminal settings
void saveterm() {
	tcgetattr(fileno(stdin), &orig_term_attr);
}

// rawterm sets the terminal to raw mode
void rawterm() {
	memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
	new_term_attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	new_term_attr.c_cc[VTIME] = 0;
	new_term_attr.c_cc[VMIN] = 0;
	tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
}

// restore resets the terminal to the previously saved setting
void restoreterm() {
	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
}

// createImageFromJpeg decompresses a JPEG image to the standard image format
// source: https://github.com/ileben/ShivaVG/blob/master/examples/test_image.c
VGImage createImageFromJpeg(const char *filename) {
	FILE *infile;
	struct jpeg_decompress_struct jdc;
	struct jpeg_error_mgr jerr;
	JSAMPARRAY buffer;
	unsigned int bstride;
	unsigned int bbpp;

	VGImage img;
	VGubyte *data;
	unsigned int width;
	unsigned int height;
	unsigned int dstride;
	unsigned int dbpp;

	VGubyte *brow;
	VGubyte *drow;
	unsigned int x;
	unsigned int lilEndianTest = 1;
	VGImageFormat rgbaFormat;

	// Check for endianness
	if (((unsigned char *)&lilEndianTest)[0] == 1)
		rgbaFormat = VG_sABGR_8888;
	else
		rgbaFormat = VG_sRGBA_8888;

	// Try to open image file
	infile = fopen(filename, "rb");
	if (infile == NULL) {
		printf("Failed opening '%s' for reading!\n", filename);
		return VG_INVALID_HANDLE;
	}
	// Setup default error handling
	jdc.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&jdc);

	// Set input file
	jpeg_stdio_src(&jdc, infile);

	// Read header and start
	jpeg_read_header(&jdc, TRUE);
	jpeg_start_decompress(&jdc);
	width = jdc.output_width;
	height = jdc.output_height;

	// Allocate buffer using jpeg allocator
	bbpp = jdc.output_components;
	bstride = width * bbpp;
	buffer = (*jdc.mem->alloc_sarray)
	    ((j_common_ptr) & jdc, JPOOL_IMAGE, bstride, 1);

	// Allocate image data buffer
	dbpp = 4;
	dstride = width * dbpp;
	data = (VGubyte *) malloc(dstride * height);

	// Iterate until all scanlines processed
	while (jdc.output_scanline < height) {

		// Read scanline into buffer
		jpeg_read_scanlines(&jdc, buffer, 1);
		drow = data + (height - jdc.output_scanline) * dstride;
		brow = buffer[0];
		// Expand to RGBA
		for (x = 0; x < width; ++x, drow += dbpp, brow += bbpp) {
			switch (bbpp) {
			case 4:
				drow[0] = brow[0];
				drow[1] = brow[1];
				drow[2] = brow[2];
				drow[3] = brow[3];
				break;
			case 3:
				drow[0] = brow[0];
				drow[1] = brow[1];
				drow[2] = brow[2];
				drow[3] = 255;
				break;
			}
		}
	}

	// Create VG image
	img = vgCreateImage(rgbaFormat, width, height, VG_IMAGE_QUALITY_BETTER);
	vgImageSubData(img, data, dstride, rgbaFormat, 0, 0, width, height);

	// Cleanup
	jpeg_destroy_decompress(&jdc);
	fclose(infile);
	free(data);

	return img;
}

// makeimage makes an image from a raw raster of red, green, blue, alpha values
void makeimage(VGfloat x, VGfloat y, int w, int h, VGubyte * data) {
	unsigned int dstride = w * 4;
	VGImageFormat rgbaFormat = VG_sABGR_8888;
	VGImage img = vgCreateImage(rgbaFormat, w, h, VG_IMAGE_QUALITY_BETTER);
	vgImageSubData(img, (void *)data, dstride, rgbaFormat, 0, 0, w, h);
	vgSetPixels(x, y, img, 0, 0, w, h);
	vgDestroyImage(img);
}

// Image places an image at the specifed location
void Image(VGfloat x, VGfloat y, int w, int h, const char *filename) {
	VGImage img = createImageFromJpeg(filename);
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

// initWindowSize requests a specific window size & position, if not called
// then init() will open a full screen window.
// Done this way to preserve the original init() behaviour.
void initWindowSize(int x, int y, unsigned int w, unsigned int h) {
	init_x = x;
	init_y = y;
	init_w = w;
	init_h = h;
}

// init sets the system to its initial state
void init(int *w, int *h) {
	bcm_host_init();
	memset(state, 0, sizeof(*state));
	state->window_x = init_x;
	state->window_y = init_y;
	state->window_width = init_w;
	state->window_height = init_h;
	oglinit(state);
	*w = state->window_width;
	*h = state->window_height;
}

// finish cleans up
void finish() {
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

// setfill sets the fill color
void setfill(VGfloat color[4]) {
	VGPaint fillPaint = vgCreatePaint();
	vgSetParameteri(fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(fillPaint, VG_PAINT_COLOR, 4, color);
	vgSetPaint(fillPaint, VG_FILL_PATH);
	vgDestroyPaint(fillPaint);
}

// setstroke sets the stroke color
void setstroke(VGfloat color[4]) {
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
	setstroke(color);
}

// Fill sets the fillcolor, defined as a RGBA quad.
void Fill(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat color[4];
	RGBA(r, g, b, a, color);
	setfill(color);
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
	setfill(color);
	setstroke(color);
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
	vgClear(0, 0, state->window_width, state->window_height);
}

// BackgroundRGB clears the screen to a background color with alpha
void BackgroundRGB(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat colour[4];
	RGBA(r, g, b, a, colour);
	vgSetfv(VG_CLEAR_COLOR, 4, colour);
	vgClear(0, 0, state->window_width, state->window_height);
}

// WindowClear clears the window to previously set background colour
void WindowClear() {
	vgClear(0, 0, state->window_width, state->window_height);
}

// AreaClear clears a given rectangle in window coordinates (not affected by
// transformations)
void AreaClear(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	vgClear(x, y, w, h);
}

// WindowOpacity sets the  window opacity
void WindowOpacity(unsigned int a) {
	dispmanChangeWindowOpacity(state, a);
}

// WindowPosition moves the window to given position
void WindowPosition(int x, int y) {
	dispmanMoveWindow(state, x, y);
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
#endif

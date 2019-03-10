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

extern void oglinit(STATE_T *);

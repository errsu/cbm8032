#include <VG/openvg.h>
#include <VG/vgu.h>
#if defined(__cplusplus)
extern "C" {
#endif
	extern void Translate(VGfloat, VGfloat);
	extern void Rotate(VGfloat);
	extern void Shear(VGfloat, VGfloat);
	extern void Scale(VGfloat, VGfloat);
	extern void Start(int, int);
	extern void End();
	extern void SaveEnd(const char *);
	extern void Background(unsigned int, unsigned int, unsigned int);
	extern void BackgroundRGB(unsigned int, unsigned int, unsigned int, VGfloat);
	extern void InitOpenVG(int *, int *);
	extern void FinishOpenVG();
	extern void Fill(unsigned int, unsigned int, unsigned int, VGfloat);
	extern void RGBA(unsigned int, unsigned int, unsigned int, VGfloat, VGfloat[4]);
	extern void RGB(unsigned int, unsigned int, unsigned int, VGfloat[4]);
	extern void ClipRect(VGint x, VGint y, VGint w, VGint h);
	extern void ClipEnd();
	extern void SaveTerm();
	extern void RestoreTerm();
	extern void RawTerm();

	// Added by Paeryn
	extern void AreaClear(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
	extern void WindowClear();
#if defined(__cplusplus)
}
#endif

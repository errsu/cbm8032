#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "graphics.h"

#define PET_GLYPH_WIDTH  16
#define PET_GLYPH_HEIGHT 24
#define PET_IMAGE_WIDTH  (16 * PET_GLYPH_WIDTH)
#define PET_IMAGE_HEIGHT (16 * PET_GLYPH_HEIGHT)

// PETSCII image contains glyphs in ROM index order
extern unsigned char petSciiImage[(PET_IMAGE_WIDTH / 8) * PET_IMAGE_HEIGHT];

VGImage makePetSciiImage() {
  unsigned int dstride = PET_IMAGE_WIDTH / 8;
  VGImage img = vgCreateImage(VG_sABGR_8888, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT, VG_IMAGE_QUALITY_BETTER);
  vgImageSubData(img, petSciiImage, dstride, VG_BW_1, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT);
  return img;
}

VGImage glyphs[256];

void prepareGlyphs(VGImage petSciiImage)
{
  for (unsigned int glyphRow = 0; glyphRow < 16; glyphRow++) {
    for (unsigned int glyphCol = 0; glyphCol < 16; glyphCol++) {
      unsigned int index = glyphRow * 16 + glyphCol;
      VGint x = glyphCol * PET_GLYPH_WIDTH;
      VGint y = PET_IMAGE_HEIGHT - ((glyphRow + 1) * PET_GLYPH_HEIGHT);
      glyphs[index] = vgChildImage(petSciiImage, x, y, PET_GLYPH_WIDTH, PET_GLYPH_HEIGHT);
    }
  }
}

void destroyGlyphs()
{
  for (unsigned int index = 0; index < 256; index++) {
    vgDestroyImage(glyphs[index]);
  }
}

static unsigned char petsciiToRomIndex[256] = {
// 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
   32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32, // 00 .. 0F
   32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32, // 10 .. 1F
   32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47, // 20 .. 2F
   48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63, // 30 .. 3F
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15, // 40 .. 4F
   16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31, // 50 .. 5F
   64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79, // 60 .. 6F
   80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95, // 70 .. 7F
   32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32, // 80 .. 8F
   32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32, // 90 .. 9F
   96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, // A0 .. AF
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, // B0 .. BF
   64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79, // C0 .. CF
   80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95, // D0 .. DF
   96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, // E0 .. EF
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, // F0 .. FF
};

// example screen encoded in PETSCII (unshifted)
// see https://en.wikipedia.org/wiki/PETSCII
static unsigned char exampleScreen[25 * 80 + 1] = "\
 1       .         .         .         .         .         .         .         .\
 2       .         .         .         .         .         .         .         .\
 3       .         .         .         .         .         .         .         .\
 4       .         .         . CBM8032 .         .         .         .         .\
 5       .         .         .         .         .         .         .         .\
 6       . ABCDEFG . HIJKLMN . OPQRSTU . VWXYZ   .         .         .         .\
 7       .         .         .         .         .         .         .         .\
 8       .         .         . 0123456 . 789     .         .         .         .\
 9       .         .         .         .         .         .         .         .\
10       . @[\\]!\"# . $%&'()* . +,-./:; . <=>?    .         .         .         .\
11       .         .         .         .         .         .         .         .\
12       .         .         .         .         .         .         .         .\
13       .         .         .         .         .         .         .         .\
14       .         .         .         .         .         .         .         .\
15       .         .         .         .         .         .         .         .\
16       .         .         .         .         .         .         .         .\
17       .         .         .         .         .         .         .         .\
18       .         .         .         .         .         .         .         .\
19       .         .         .         .         .         .         .         .\
20       .         .         .         .         .         .         .         .\
21       .         .         .         .         .         .         .         .\
22       .         .         .         .         .         .         .         .\
23       .         .         .         .         .         .         .         .\
24       .         .         .         .         .         .         .         .\
25       .         .         .         .         .         .         .         .";

static unsigned char screenContent[25 * 80]; // holds rom indices

void imagetest(int screenW, int screenH) {

  for (unsigned int i = 0; i < 25 * 80; i++) {
    screenContent[i] = petsciiToRomIndex[exampleScreen[i]];
  }

  int imgW = PET_IMAGE_WIDTH;
  int imgH = PET_IMAGE_HEIGHT;

  Start(screenW, screenH);
  Background(0, 0, 0);
  VGPaint paint = vgCreatePaint();
  VGfloat paintColor[4] = { 0.1f, 0.7f, 0.2f, 1.0f };
  vgSetParameterfv(paint, VG_PAINT_COLOR, 4, paintColor);
  vgSetPaint(paint, VG_FILL_PATH);
  VGImage img = makePetSciiImage();
  prepareGlyphs(img);
  // VGImage glyph = vgChildImage(img, 0, PET_IMAGE_HEIGHT - PET_GLYPH_HEIGHT, PET_GLYPH_WIDTH, PET_GLYPH_HEIGHT);
  vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
  vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_MULTIPLY);

  vgLoadIdentity();
  vgTranslate(screenW - imgW, screenH - imgH);
  vgDrawImage(img);

  // draw screen in lower left corner
  vgLoadIdentity();
  vgTranslate(0, PET_GLYPH_HEIGHT * 24);

  for (unsigned int row = 0; row < 25; row++) {
    for (unsigned int col = 0; col < 80; col++) {
      unsigned int index = row * 80 + col;
      unsigned int cbmCode = screenContent[index];
      vgDrawImage(glyphs[cbmCode]);
      vgTranslate(PET_GLYPH_WIDTH, 0);
    }
    vgTranslate(-PET_GLYPH_WIDTH * 80, -PET_GLYPH_HEIGHT);
  }

  destroyGlyphs();
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
  SaveTerm();
  InitOpenVG(&w, &h);
  RawTerm();
  imagetest(w, h);
  waituntil(0x1b);
  RestoreTerm();
  FinishOpenVG();
  return 0;
}

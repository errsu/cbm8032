#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "graphics.h"

#define SCREENW 1280
#define SCREENH 720

#define PET_GLYPH_WIDTH  16
#define PET_GLYPH_HEIGHT 24
#define PET_IMAGE_WIDTH  (16 * PET_GLYPH_WIDTH)
#define PET_IMAGE_HEIGHT (16 * PET_GLYPH_HEIGHT)

// #define IMAGE_QUALITY VG_IMAGE_QUALITY_BETTER
#define IMAGE_QUALITY VG_IMAGE_QUALITY_NONANTIALIASED
// #define PIXEL_FORMAT VG_sXBGR_8888
#define PIXEL_FORMAT VG_sRGB_565


// PETSCII image contains glyphs in ROM index order
extern unsigned char petSciiImageData[(PET_IMAGE_WIDTH / 8) * PET_IMAGE_HEIGHT];

// uint32_t rgb_data[PET_IMAGE_WIDTH*PET_IMAGE_HEIGHT];
uint16_t rgb_data[PET_IMAGE_WIDTH*PET_IMAGE_HEIGHT];

VGImage petSciiImage;

void makePetSciiImage() {
  unsigned int dstride = PET_IMAGE_WIDTH / 8;
  VGImage imgTemp = vgCreateImage(PIXEL_FORMAT, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT, IMAGE_QUALITY);
  vgImageSubData(imgTemp, petSciiImageData, dstride, VG_BW_1, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT);
  petSciiImage = vgCreateImage(PIXEL_FORMAT, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT, IMAGE_QUALITY);
  vgCopyImage(petSciiImage, 0, 0, imgTemp, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT, VG_FALSE);
  vgDestroyImage(imgTemp);

  // set green color
  // vgGetImageSubData(petSciiImage, &rgb_data, PET_IMAGE_WIDTH * sizeof(uint32_t), PIXEL_FORMAT, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT);
  // for (unsigned i = 0; i < PET_IMAGE_WIDTH * PET_IMAGE_HEIGHT * 3; i++) {
  //   if (rgb_data[i*3] != 0x00) {
  //     rgb_data[i*3] = 0xFF40E030; // XBGR
  //   }
  // }
  // vgImageSubData(petSciiImage, &rgb_data, PET_IMAGE_WIDTH * sizeof(uint32_t), PIXEL_FORMAT, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT);


  vgGetImageSubData(petSciiImage, &rgb_data, PET_IMAGE_WIDTH * sizeof(uint16_t), PIXEL_FORMAT, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT);
  for (unsigned i = 0; i < PET_IMAGE_WIDTH * PET_IMAGE_HEIGHT; i++) {
    if (rgb_data[i] != 0x0000) {
      rgb_data[i] = 0x4706;
    }
  }
  vgImageSubData(petSciiImage, &rgb_data, PET_IMAGE_WIDTH * sizeof(uint16_t), PIXEL_FORMAT, 0, 0, PET_IMAGE_WIDTH, PET_IMAGE_HEIGHT);
}

void destroyPetSciiImage() {
  vgDestroyImage(petSciiImage);
}

VGImage glyphs[256];

void prepareGlyphs() {
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
//  0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
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
18       .         . \xA6\xA6      .         .         .         .         .         .\
19       .         . \xA6\xA6      .         .         .         .         .         .\
20       .         .         .         .         .         .         .         .\
21       .         .         .         .         .         .         .         .\
22       .         .         .         .         .         .         .         .\
23       .         .         .         .         .         .         .         .\
24       .         .         .         .         .         .         .         .\
25       .         .         .         .         .         .         .         .";

static unsigned char screenContent[25 * 80]; // holds rom indices

// uint32_t screenData[SCREENW*SCREENH];
// VGImage screenImage;
// VGImage screenImage0, screenImage1;

void prepareImageTest()
{
  makePetSciiImage();
  prepareGlyphs();
  // screenImage = vgCreateImage(VG_sXBGR_8888, SCREENW, SCREENH, IMAGE_QUALITY);
  // screenImage0 = vgCreateImage(VG_sXBGR_8888, SCREENW, SCREENH, IMAGE_QUALITY);
  // screenImage1 = vgCreateImage(VG_sXBGR_8888, SCREENW, SCREENH, IMAGE_QUALITY);
  // for (unsigned i = 0; i < SCREENW*SCREENH; i++) {
  //   screenData[i] = 0xFF000000;
  // }
  // vgImageSubData(screenImage0, screenData, SCREENW * sizeof(uint32_t), VG_sXBGR_8888, 0, 0, SCREENW, SCREENH);
  // for (unsigned i = 0; i < SCREENW*SCREENH; i++) {
  //   screenData[i] = 0xFF40E030;
  // }
  // vgImageSubData(screenImage1, screenData, SCREENW * sizeof(uint32_t), VG_sXBGR_8888, 0, 0, SCREENW, SCREENH);
}

void finishImageTest()
{
  // vgDestroyImage(screenImage0);
  // vgDestroyImage(screenImage1);
  // vgDestroyImage(screenImage);
  destroyGlyphs();
  destroyPetSciiImage();
}

void imageTest(int screenW, int screenH, unsigned frame) {

  for (unsigned int i = 0; i < 25 * 80; i++) {
    exampleScreen[32] = (unsigned char)(' ' + (frame % 32));
    screenContent[i] = petsciiToRomIndex[exampleScreen[i]];
  }

  Start(screenW, screenH);

  // uint32_t color = (frame % 2 == 0) ? 0xFF000000 : 0xFF40E030;
  // for (unsigned i = 0; i < SCREENW*SCREENH; i++) {
  //   screenData[i] = color;
  // }
  // vgImageSubData(screenImage, screenData, SCREENW * sizeof(uint32_t), VG_sXBGR_8888, 0, 0, SCREENW, SCREENH);
  // if (frame % 2 == 0) {
  //   vgSetPixels(0, 0, screenImage0, 0, 0, SCREENW, SCREENH);
  // } else {
  //   vgSetPixels(0, 0, screenImage1, 0, 0, SCREENW, SCREENH);
  // }
  // vgSetPixels(0, 0, screenImage, 0, 0, SCREENW, SCREENH);
  // if (frame % 2 == 0) {
  //     Background(0, 0, 0);
  // } else {
  //     Background(255, 255, 255);
  // }
  Background(0, 0, 0);

  vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
  vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
  vgSeti(VG_IMAGE_QUALITY, IMAGE_QUALITY); // antialiasing doesn't take extra time

  // VGfloat scaleX = 24.0f / 16.0f;
  // VGfloat scaleY = 36.0f / 24.0f;

  for (int row = 0; row < 25; row++) {
    for (int col = 0; col < 80; col++) {
      int index = row * 80 + col;
      unsigned char cbmCode = screenContent[index];
      vgLoadIdentity();
      // VGfloat dx = (VGfloat)screenW / 2.0f + scaleX * (VGfloat)(PET_GLYPH_WIDTH * (col - 40));
      // VGfloat dy = (VGfloat)screenH / 2.0f + scaleY * (VGfloat)(PET_GLYPH_HEIGHT * (12 - row));
      VGfloat dx = (VGfloat)SCREENW / 2.0f + (VGfloat)(PET_GLYPH_WIDTH * (col - 40));
      VGfloat dy = (VGfloat)SCREENH / 2.0f + (VGfloat)(PET_GLYPH_HEIGHT * (12 - row));
      vgTranslate(dx, dy);
      // vgScale(scaleX, scaleY);
      vgDrawImage(glyphs[cbmCode]);
    }
  }

  End();
}

// main initializes the system and shows the picture.
// Exit and clean up when you hit [RETURN].
int main(int argc, char **argv) {
  int w, h;
  SaveTerm();
  InitOpenVG(&w, &h);
  RawTerm();
  prepareImageTest(w, h);
  for (unsigned i = 0; i < 600; i++) {
    imageTest(w, h, i);
  }
  finishImageTest();
  RestoreTerm();
  FinishOpenVG();
  return 0;
}

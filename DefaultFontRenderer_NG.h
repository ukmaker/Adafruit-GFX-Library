#ifndef _ADAFRUIT_DefaultFontRenderer_NG_H
#define _ADAFRUIT_DefaultFontRenderer_NG_H

#if ARDUINO >= 100
#include "Arduino.h"
#include "Print.h"
#else
#include "WProgram.h"
#endif
#include "FontRenderer.h"
#include "STMDMA.h"
#include "gfxfont.h"

class Adafruit_GFX_NG;

class DefaultFontRenderer_NG : public FontRenderer {
 public:
  DefaultFontRenderer_NG(const GFXfont *font, int16_t w, int16_t h);
  ~DefaultFontRenderer_NG() {}

  /**********************************************************************/
  /*!
    @brief   Set a clipping window for text
    @note    Affects calls to write() only, and hence also to print()
             Calls to drawChar() are not affected
  */
  /**********************************************************************/
  void setTextWindow(int16_t x, int16_t y, int16_t w, int16_t h);
  void removeTextWindow();

  virtual void getTextBounds(const char *string, int16_t x, int16_t y,
                             int16_t *x1, int16_t *y1, uint16_t *w,
                             uint16_t *h);
  virtual void getTextBounds(const __FlashStringHelper *s, int16_t x, int16_t y,
                             int16_t *x1, int16_t *y1, uint16_t *w,
                             uint16_t *h);

  virtual size_t write(Adafruit_GFX_NG &gfx, uint8_t c);

  virtual void drawChar(Adafruit_GFX_NG &gfx, int16_t x, int16_t y,
                        unsigned char c, uint16_t color, uint16_t bg,
                        uint8_t size_x, uint8_t size_y);

  virtual void setFont(const GFXfont *f);
  virtual uint16_t getFontHeight();
  virtual void charBounds(unsigned char c, int16_t *x, int16_t *y,
                          int16_t *minx, int16_t *miny, int16_t *maxx,
                          int16_t *maxy);

  void setBlitter(STMDMA *stmdma);

 protected:
  bool textWindowed;  ///< Should text windowing be applied
  int16_t textX;      ///< Left side of text window
  int16_t textW;      ///< Width of text window
  int16_t textY;      ///< Top of text window
  int16_t textH;      ///< Height of text window
  int8_t ymin, ymax;  // Max and min y-offsets from zero for any character in the current font
  STMDMA *_stmdma;
};

#endif  // _ADAFRUIT_DefaultFontRenderer_NG_H
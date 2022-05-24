#include "DefaultFontRenderer_NG.h"

#include "Adafruit_SPITFT_NG.h"
#include "adafruit_pgmspace.h"

// NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
// THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
// has typically been used with the 'classic' font to overwrite old
// screen contents with new data.  This ONLY works because the
// characters are a uniform size; it's not a sensible thing to do with
// proportionally-spaced fonts with glyphs of varying sizes (and that
// may overlap).  To replace previously-drawn text when using a custom
// font, use the getTextBounds() function to determine the smallest
// rectangle encompassing a string, erase the area with fillRect(),
// then draw new text.  This WILL infortunately 'blink' the text, but
// is unavoidable.  Drawing 'background' pixels will NOT fix this,
// only creates a new set of problems.  Have an idea to work around
// this (a canvas object type for MCUs that can afford the RAM and
// displays supporting setAddrWindow() and pushColors()), but haven't
// implemented this yet.

DefaultFontRenderer_NG::DefaultFontRenderer_NG(const GFXfont *font, int16_t w,
                                               int16_t h)
    : FontRenderer(w, h) {
  gfxFont = font;
  textWindowed = false;
  textX = textY = textW = textH = 0;
  _stmdma = nullptr;
}
// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color,
   no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void DefaultFontRenderer_NG::drawChar(Adafruit_GFX_NG &gfx, int16_t x,
                                      int16_t y, unsigned char c,
                                      uint16_t color, uint16_t bg,
                                      uint8_t size_x, uint8_t size_y) {
  // Character is assumed previously filtered by write() to eliminate
  // newlines, returns, non-printable characters, etc.  Calling
  // drawChar() directly with 'bad' characters of font may cause mayhem!

  c -= (uint8_t)pgm_read_byte(&gfxFont->first);
  GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
  uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

  uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
  uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
  int8_t xo = pgm_read_byte(&glyph->xOffset),
         yo = pgm_read_byte(&glyph->yOffset);
  uint8_t xx, yy, bits = 0, bit = 0;
  int16_t xo16 = 0, yo16 = 0;

  if (size_x > 1 || size_y > 1) {
    xo16 = xo;
    yo16 = yo;
  }

  // Todo: Add character clipping here

  if (_stmdma != nullptr) {
    _stmdma->beginWindow(x + xo, y + yo, w, h, bg);
  } else {
      gfx.startWrite();
  }
  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = pgm_read_byte(&bitmap[bo++]);
      }
      if (bits & 0x80) {
        if (size_x == 1 && size_y == 1) {
          if (_stmdma != nullptr) {
            _stmdma->drawWindowPixel(xx, yy, color);
          } else {
            gfx.writePixel(x + xo + xx, y + yo + yy, color);
          }
        } else {
          gfx.writeFillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y,
                            size_x, size_y, color);
        }
      }
      bits <<= 1;
    }
  }
  if(_stmdma != nullptr) {
    _stmdma->flushWindow();
    _stmdma->waitComplete();
  }
  gfx.endWrite();
}
/**************************************************************************/
/*!
    @brief  Print one byte/character of data, used to support print()
    @param  c  The 8-bit ascii character to write
    @note If text windowing is enabled then any character which exceeds the
    bounds will not be printed at all - the whole character is clipped.
/**************************************************************************/
size_t DefaultFontRenderer_NG::write(Adafruit_GFX_NG &gfx, uint8_t c) {
  int16_t x0, y0, xw, yh;
  if (textWindowed) {
    x0 = textX;
    xw = textW;
    y0 = textY;
    yh = textH;
  } else {
    x0 = 0;
    xw = _width;
    y0 = 0;
    yh = _height;
  }

  if (c == '\n') {
    cursor_x = x0;
    cursor_y +=
        (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
  } else if (c != '\r') {
    uint8_t first = pgm_read_byte(&gfxFont->first);
    if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
      GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
      uint8_t w = pgm_read_byte(&glyph->width),
              h = pgm_read_byte(&glyph->height);
      if ((w > 0) && (h > 0)) {  // Is there an associated bitmap?
        int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset);  // sic
        uint16_t cw = textsize_x * (xo + w);
        if (wrap && ((cursor_x + cw) > (x0 + xw))) {
          cursor_x = x0;
          cursor_y +=
              (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        }
        if (!textWindowed || (textWindowed && ((cursor_x + cw) <= (x0 + xw)) &&
                              (cursor_y <= (y0 + yh)))) {
          drawChar(gfx, cursor_x, cursor_y, c, textcolor, textbgcolor,
                   textsize_x, textsize_y);
        }
      }
      cursor_x +=
          (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
    }
  }
  return 1;
}

/**************************************************************************/
/*!
    @brief  Set a window to print text in, clip lines which are too long
    @param  x  x coordinate of the window's top left point
    @param  y  y coordinate of the window's top left point
    @param  w  The width of the window
    @param  h  The height of the window
    @note Also sets the text cursor position to the top left
          character position (y + the font height)
/**************************************************************************/
void DefaultFontRenderer_NG::setTextWindow(int16_t x, int16_t y, int16_t w,
                                           int16_t h) {
  textX = cursor_x = x;
  textY = y;
  cursor_y = y + textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance) - 1;
  textW = w;
  textH = h;
  textWindowed = true;
}
/**************************************************************************/
/*!
    @brief  Disables text windowing
/**************************************************************************/
void DefaultFontRenderer_NG::removeTextWindow() { textWindowed = false; }

void DefaultFontRenderer_NG::getTextBounds(const char *string, int16_t x,
                                           int16_t y, int16_t *x1, int16_t *y1,
                                           uint16_t *w, uint16_t *h) {
  FontRenderer::getTextBounds(string, x, y, x1, y1, w, h);
  if (textWindowed) {
    if (*x1 < textX) *x1 = textX;
    if (*w > textW) *w = textW;
    if (*y1 < textY) *y1 = textY;
    if (*h > textH) *h = textH;
  }
}
void DefaultFontRenderer_NG::getTextBounds(const __FlashStringHelper *s,
                                           int16_t x, int16_t y, int16_t *x1,
                                           int16_t *y1, uint16_t *w,
                                           uint16_t *h) {
  FontRenderer::getTextBounds(s, x, y, x1, y1, w, h);

  if (textWindowed) {
    if (*x1 < textX) *x1 = textX;
    if (*w > textW) *w = textW;
    if (*y1 < textY) *y1 = textY;
    if (*h > textH) *h = textH;
  }
}

/**************************************************************************/
/*!
    @brief Set the font to display when print()ing, either custom or default
    @param  f  The GFXfont object, if NULL use built in 6x8 font
*/
/**************************************************************************/
void DefaultFontRenderer_NG::setFont(const GFXfont *f) {
  gfxFont = (GFXfont *)f;
}

/**************************************************************************/
/*!
    @brief  Helper to determine size of a character with current font/size.
            Broke this out as it's used by both the PROGMEM- and RAM-resident
            getTextBounds() functions.
    @param  c     The ASCII character in question
    @param  x     Pointer to x location of character. Value is modified by
                  this function to advance to next character.
    @param  y     Pointer to y location of character. Value is modified by
                  this function to advance to next character.
    @param  minx  Pointer to minimum X coordinate, passed in to AND returned
                  by this function -- this is used to incrementally build a
                  bounding rectangle for a string.
    @param  miny  Pointer to minimum Y coord, passed in AND returned.
    @param  maxx  Pointer to maximum X coord, passed in AND returned.
    @param  maxy  Pointer to maximum Y coord, passed in AND returned.
*/
/**************************************************************************/
void DefaultFontRenderer_NG::charBounds(unsigned char c, int16_t *x, int16_t *y,
                                        int16_t *minx, int16_t *miny,
                                        int16_t *maxx, int16_t *maxy) {
  if (c == '\n') {  // Newline?
    *x = 0;         // Reset x to zero, advance y by one line
    *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
  } else if (c != '\r') {  // Not a carriage return; is normal char
    uint8_t first = pgm_read_byte(&gfxFont->first),
            last = pgm_read_byte(&gfxFont->last);
    if ((c >= first) && (c <= last)) {  // Char present in this font?
      GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
      uint8_t gw = pgm_read_byte(&glyph->width),
              gh = pgm_read_byte(&glyph->height),
              xa = pgm_read_byte(&glyph->xAdvance);
      int8_t xo = pgm_read_byte(&glyph->xOffset),
             yo = pgm_read_byte(&glyph->yOffset);
      if (wrap && ((*x + (((int16_t)xo + gw) * textsize_x)) > _width)) {
        *x = 0;  // Reset x to zero, advance y by one line
        *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
      }
      int16_t tsx = (int16_t)textsize_x, tsy = (int16_t)textsize_y,
              x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + xa * tsx - 1,
              y2 = y1 + gh * tsy - 1;
      if (x1 < *minx) *minx = x1;
      if (y1 < *miny) *miny = y1;
      if (x2 > *maxx) *maxx = x2;
      if (y2 > *maxy) *maxy = y2;
      *x += xa * tsx;
    }
  }
}

/**************************************************************************/
/*!
    @brief  Helper to determine the height in pixels of the current font
    @returns The height of the font
/**************************************************************************/
uint16_t DefaultFontRenderer_NG::getFontHeight() {
  return (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
}

void DefaultFontRenderer_NG::setBlitter(STMDMA *stmdma) { _stmdma = stmdma; }
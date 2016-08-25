
#pragma once

#include "msdfgen/core/Shape.h"

typedef struct FT_FaceRec_*  FT_Face;

namespace msdfgen {

/// Returns the size of one EM in the font's coordinate system
bool getFontScale(double &output, FT_Face face);
/// Returns the width of space and tab
bool getFontWhitespaceWidth(double &spaceAdvance, double &tabAdvance, FT_Face face);
/// Loads the shape prototype of a glyph from font file using the glyph index
bool loadGlyph(Shape &output, FT_Face face, unsigned int glyphIndex, double *advance = NULL, bool printInfo = false);
/// Loads the shape prototype of a glyph from font file using a character code
bool loadChar(Shape &output, FT_Face face, unsigned long charCode, double *advance = NULL);
/// Returns the kerning distance adjustment between two specific glyphs.
bool getKerning(double &output, FT_Face face, int unicode1, int unicode2);

} // namespace msdfgen
#include "msdfgen/util.h"

#include "cinder/Log.h"

#include "ft2build.h"
#include FT_FREETYPE_H

#define REQUIRE(cond) { if (!(cond)) return false; }

namespace msdfgen {

bool getFontScale(double &output, FT_Face face) {
    output = face->units_per_EM/64.;
    return true;
}

bool getFontWhitespaceWidth(double &spaceAdvance, double &tabAdvance, FT_Face face) {
    FT_Error error = FT_Load_Char(face, ' ', FT_LOAD_NO_SCALE);
    if (error)
        return false;
    spaceAdvance = face->glyph->advance.x/64.;
    error = FT_Load_Char(face, '\t', FT_LOAD_NO_SCALE);
    if (error)
        return false;
    tabAdvance = face->glyph->advance.x/64.;
    return true;
}

bool loadGlyph(Shape &output, FT_Face face, unsigned int glyphIndex, double *advance, bool printInfo) {
    enum PointType {
        NONE = 0,
        PATH_POINT,
        QUADRATIC_POINT,
        CUBIC_POINT,
        CUBIC_POINT2
    };

    if (nullptr == face)
        return false;
    FT_Error error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_SCALE);
    if (error)
        return false;
    output.contours.clear();
    output.inverseYAxis = false;
    if (advance)
        *advance = face->glyph->advance.x/64.;

    float glyphScale = 2048.0f / face->units_per_EM;

    int last = -1;
    // For each contour
    for (int i = 0; i < face->glyph->outline.n_contours; ++i) {

        Contour &contour = output.addContour();
        int first = last+1;
        int firstPathPoint = -1;
        last = face->glyph->outline.contours[i];

        PointType state = NONE;
        Point2 startPoint;
        Point2 controlPoint[2];

        // For each point on the contour
        for (int round = 0, index = first; round == 0; ++index) {
            if (index > last) {
                REQUIRE(firstPathPoint >= 0);
                index = first;
            }
            // Close contour
            if (index == firstPathPoint)
                ++round;

            Point2 point( glyphScale * face->glyph->outline.points[index].x/64., glyphScale * face->glyph->outline.points[index].y/64.);
            PointType pointType = face->glyph->outline.tags[index]&1 ? PATH_POINT : face->glyph->outline.tags[index]&2 ? CUBIC_POINT : QUADRATIC_POINT;

            switch (state) {
                case NONE:
                    if (pointType == PATH_POINT) {
                        firstPathPoint = index;
                        startPoint = point;
                        state = PATH_POINT;
                    } else if((face->glyph->outline.tags[first] == FT_Curve_Tag_Conic) && (face->glyph->outline.tags[last] == FT_Curve_Tag_Conic)) {
                        firstPathPoint = index;
						Point2 firstPoint( glyphScale * face->glyph->outline.points[first].x/64., glyphScale * face->glyph->outline.points[first].y/64.);
						Point2 lastPoint( glyphScale * face->glyph->outline.points[last].x/64., glyphScale * face->glyph->outline.points[last].y/64.);
						startPoint = .5*(firstPoint + lastPoint);
                        controlPoint[0] = point;						
                        state = QUADRATIC_POINT;
					}
                    break;
                case PATH_POINT:
                    if (pointType == PATH_POINT) {
                        contour.addEdge(new LinearSegment(startPoint, point));
                        startPoint = point;
                    } else {
                        controlPoint[0] = point;
                        state = pointType;
                    }
                    break;
                case QUADRATIC_POINT:
                    REQUIRE(pointType != CUBIC_POINT);
                    if (pointType == PATH_POINT) {
                        contour.addEdge(new QuadraticSegment(startPoint, controlPoint[0], point));
                        startPoint = point;
                        state = PATH_POINT;
                    } else {
                        Point2 midPoint = .5*controlPoint[0]+.5*point;
                        contour.addEdge(new QuadraticSegment(startPoint, controlPoint[0], midPoint));
                        startPoint = midPoint;
                        controlPoint[0] = point;

                    }
                    break;
                case CUBIC_POINT:
                    REQUIRE(pointType == CUBIC_POINT);
                    controlPoint[1] = point;
                    state = CUBIC_POINT2;
                    break;
                case CUBIC_POINT2:
                    REQUIRE(pointType != QUADRATIC_POINT);
                    if (pointType == PATH_POINT) {
                        contour.addEdge(new CubicSegment(startPoint, controlPoint[0], controlPoint[1], point));
                        startPoint = point;
                    } else {
                        Point2 midPoint = .5*controlPoint[1]+.5*point;
                        contour.addEdge(new CubicSegment(startPoint, controlPoint[0], controlPoint[1], midPoint));
                        startPoint = midPoint;
                        controlPoint[0] = point;
                    }
                    state = pointType;
                    break;
            }

        }
    }
    return true;
}

bool loadChar(Shape &output, FT_Face face, unsigned int charCode, double *advance) {
	unsigned int glyphIndex = FT_Get_Char_Index(face, charCode);
	return loadGlyph(output, face, glyphIndex, advance );
}

bool getKerning(double &output, FT_Face face, int charCode1, int charCode2) {
    FT_Vector kerning;
    if (FT_Get_Kerning(face, FT_Get_Char_Index(face, charCode1), FT_Get_Char_Index(face, charCode2), FT_KERNING_UNSCALED, &kerning)) {
        output = 0;
        return false;
    }
    output = kerning.x/64.;
    return true;
}

} // namespace msdfgen
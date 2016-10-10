/*
Copyright 2016 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Copyright (c) 2016, The Cinder Project, All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org
Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/gl/SdfText.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/scoped.h"
#include "cinder/ip/Fill.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"
#include "cinder/Text.h"
#include "cinder/Unicode.h"
#include "cinder/Utilities.h"

#include "cinder/app/App.h"

#if defined( CINDER_COCOA )
	#include "cinder/cocoa/CinderCocoa.h"
	#if defined( CINDER_COCOA_TOUCH )
		#import <UIKit/UIKit.h>
		#import <CoreText/CoreText.h>
	#else
		#import <Cocoa/Cocoa.h>
	#endif
#endif

#include "ft2build.h"
#include FT_FREETYPE_H
#include "freetype/ftsnames.h"
#include "freetype/ttnameid.h"

#include "msdfgen/msdfgen.h"
#include "msdfgen/util.h"

#include <cmath>
#include <set>
#include <vector>
#include <boost/algorithm/string.hpp>

#if defined( CINDER_LINUX )
	#include <fontconfig/fontconfig.h>
#elif defined( CINDER_MSW )
	#include <Windows.h>
#endif

static const float MAX_SIZE = 1000000.0f;

namespace cinder { namespace gl {

#if defined( CINDER_GL_ES )
static std::string kSdfVertShader =
	"#version 100\n"
	"precision mediump float;\n"
	"uniform mat4 ciModelViewProjection;\n"
	"attribute vec4 ciPosition;\n"
	"attribute vec2 ciTexCoord0;\n"
	"varying vec2 TexCoord;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = ciModelViewProjection * ciPosition;\n"
	"	TexCoord = ciTexCoord0;\n"
	"}\n";

static std::string kSdfFragShader =
	"#version 100\n"
	"#extension GL_OES_standard_derivatives : enable\n"
	"precision mediump float;\n"
	"precision mediump sampler2D;\n"
	"uniform sampler2D uTex0;\n"
	"uniform vec2      uTexSize;\n"
	"uniform vec4      uFgColor;\n"
	"uniform float     uPremultiply;\n"
	"uniform float     uGamma;\n"
	"varying vec2      TexCoord;\n"
	"\n"
	"float median( float r, float g, float b ) {\n"
	"	return max( min( r, g ), min( max( r, g ), b ) );\n"
	"}\n"
	"\n"
	"vec2 safeNormalize( in vec2 v ) {\n"
	"   float len = length( v );\n"
	"   len = ( len > 0.0 ) ? 1.0 / len : 0.0;\n"
	"   return v * len;\n"
	"}\n"
	"\n"
  #if defined( CINDER_LINUX_EGL_ONLY )
	"float calcDiff( vec2 p ) {\n"
	"   return p.x * p.x - p.y;\n"
	"}\n"
	"\n"	
	"void main(void) {\n"
	"    vec3 sample = texture2D( uTex0, TexCoord ).rgb;\n"
	"    float sigDist = median( sample.r, sample.g, sample.b );\n"
	"    float c = calcDiff( TexCoord );\n"
	"    vec2 ps = vec2( 1.0 / uTexSize.x, 1.0 / uTexSize.y );\n"
	"    float dfdx = calcDiff( TexCoord + vec2( ps.x ) ) - c;\n"
	"    float dfdy = calcDiff( TexCoord + vec2( ps.y ) ) - c;\n"
	"    float w = abs( dfdx ) + abs( dfdy );\n"
	"    float opacity = smoothstep( 0.5 - w, 0.5 + w, sigDist );\n"
  #else
	"void main(void) {\n"
	"    // Convert normalized texcoords to absolute texcoords.\n"
	"    vec2 uv = TexCoord * uTexSize;\n"
	"    // Calculate derivates\n"
	"    vec2 Jdx = dFdx( uv );\n"
	"    vec2 Jdy = dFdy( uv );\n"
	"    // Sample SDF texture (3 channels).\n"
	"    vec3 sample = texture2D( uTex0, TexCoord ).rgb;\n"
	"    // Calculate signed distance (in texels).\n"
	"    float sigDist = median( sample.r, sample.g, sample.b ) - 0.5;\n"
	"    // For proper anti-aliasing, we need to calculate signed distance in pixels. We do this using derivatives.\n"
	"    vec2 gradDist = safeNormalize( vec2( dFdx( sigDist ), dFdy( sigDist ) ) );\n"
	"    vec2 grad = vec2( gradDist.x * Jdx.x + gradDist.y * Jdy.x, gradDist.x * Jdx.y + gradDist.y * Jdy.y );\n"
	"    // Apply anti-aliasing.\n"
	"    const float kThickness = 0.125;\n"
	"    float kNormalization = kThickness * 0.5 * sqrt( 2.0 );\n"
	"    float afwidth = min( kNormalization * length( grad ), 0.5 );\n"
	"    float opacity = smoothstep( 0.0 - afwidth, 0.0 + afwidth, sigDist );\n"
  #endif
	"    // If enabled apply pre-multiplied alpha. Always apply gamma correction.\n"
	"    vec4 color;\n"
	"    color.a = pow( uFgColor.a * opacity, 1.0 / uGamma );\n"
	"    color.rgb = mix( uFgColor.rgb, uFgColor.rgb * color.a, uPremultiply );\n"
	"    gl_FragColor = color;\n"
	"}\n";
#else
static std::string kSdfVertShader = 
	"#version 150\n"
	"uniform mat4 ciModelViewProjection;\n"
	"in vec4 ciPosition;\n"
	"in vec2 ciTexCoord0;\n"
	"out vec2 TexCoord;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = ciModelViewProjection * ciPosition;\n"
	"	TexCoord = ciTexCoord0;\n"
	"}\n";

static std::string kSdfFragShader = 
	"#version 150\n"
	"uniform sampler2D uTex0;\n"
	"uniform vec4      uFgColor;\n"
	"uniform float     uPremultiply;\n"
	"uniform float     uGamma;\n"
	"in vec2           TexCoord;\n"
	"out vec4          Color;\n"
	"\n"
	"float median( float r, float g, float b ) {\n"
	"	return max( min( r, g ), min( max( r, g ), b ) );\n"
	"}\n"
	"\n"
	"vec2 safeNormalize( in vec2 v ) {\n"
	"   float len = length( v );\n"
	"   len = ( len > 0.0 ) ? 1.0 / len : 0.0;\n"
	"   return v * len;\n"
	"}\n"
	"\n"
	"void main(void) {\n"
	"    // Convert normalized texcoords to absolute texcoords.\n"
	"    vec2 uv = TexCoord * textureSize( uTex0, 0 );\n"
	"    // Calculate derivates\n"
	"    vec2 Jdx = dFdx( uv );\n"
	"    vec2 Jdy = dFdy( uv );\n"
	"    // Sample SDF texture (3 channels).\n"
	"    vec3 sample = texture( uTex0, TexCoord ).rgb;\n"
	"    // Calculate signed distance (in texels).\n"
	"    float sigDist = median( sample.r, sample.g, sample.b ) - 0.5;\n"
	"    // For proper anti-aliasing, we need to calculate signed distance in pixels. We do this using derivatives.\n"
	"    vec2 gradDist = safeNormalize( vec2( dFdx( sigDist ), dFdy( sigDist ) ) );\n"
	"    vec2 grad = vec2( gradDist.x * Jdx.x + gradDist.y * Jdy.x, gradDist.x * Jdx.y + gradDist.y * Jdy.y );\n"
	"    // Apply anti-aliasing.\n"
	"    const float kThickness = 0.125;\n"
	"    const float kNormalization = kThickness * 0.5 * sqrt( 2.0 );\n"
	"    float afwidth = min( kNormalization * length( grad ), 0.5 );\n"
	"    float opacity = smoothstep( 0.0 - afwidth, 0.0 + afwidth, sigDist );\n"
    "    // If enabled apply pre-multiplied alpha. Always apply gamma correction.\n"
	"    Color.a = pow( uFgColor.a * opacity, 1.0 / uGamma );\n"
	"    Color.rgb = mix( uFgColor.rgb, uFgColor.rgb * Color.a, uPremultiply );\n"
//	"    Color = vec4( 1, 0, 0, 1 );\n"
	"}\n";
#endif

static gl::GlslProgRef sDefaultShader;

// =================================================================================================
// SdfText::TextureAtlas
// =================================================================================================
class SdfText::TextureAtlas {
public:

	struct CacheKey {
		std::string mFamilyName;
		std::string mStyleName;
		std::string mUtf8Chars;
		ivec2		mTextureSize = ivec2( 0 );
		ivec2		mSdfBitmapSize = ivec2( 0 );
		bool operator==( const CacheKey& rhs ) const { 
			return ( mFamilyName == rhs.mFamilyName ) &&
				   ( mStyleName == rhs.mStyleName ) && 
				   ( mUtf8Chars == rhs.mUtf8Chars ) &&
				   ( mTextureSize == rhs.mTextureSize ) &&
				   ( mSdfBitmapSize == rhs.mSdfBitmapSize );
		}
		bool operator!=( const CacheKey& rhs ) const {
			return ( mFamilyName != rhs.mFamilyName ) ||
				   ( mStyleName != rhs.mStyleName ) || 
				   ( mUtf8Chars != rhs.mUtf8Chars ) ||
				   ( mTextureSize != rhs.mTextureSize ) ||
				   ( mSdfBitmapSize != rhs.mSdfBitmapSize );
		}
	};

	typedef std::vector<std::pair<CacheKey, SdfText::TextureAtlasRef>> AtlasCacher;

	// ---------------------------------------------------------------------------------------------

	virtual ~TextureAtlas() {}

	static SdfText::TextureAtlasRef create( FT_Face face, const SdfText::Format &format, const std::vector<SdfText::Font::Glyph> &glyphIndices );

	static ivec2 calculateSdfBitmapSize( const vec2 &sdfScale, const ivec2& sdfPadding, const vec2 &maxGlyphSize );

private:
	TextureAtlas();
	TextureAtlas( FT_Face face, const SdfText::Format &format, const std::vector<SdfText::Font::Glyph> &glyphIndices );
	friend class SdfText;

	FT_Face							mFace = nullptr;
	std::vector<gl::TextureRef>		mTextures;
	SdfText::Font::GlyphInfoMap		mGlyphInfo;

	//! Base scale that SDF generator uses is size 32 at 72 DPI. A scale of 1.5, 2.0, and 3.0 translates to size 48, 64 and 96 and 72 DPI.
	vec2						mSdfScale = vec2( 1.0f );
	vec2						mSdfPadding = vec2( 2.0f );
	ivec2						mSdfBitmapSize = ivec2( 0 );
	vec2						mMaxGlyphSize = vec2( 0.0f );
	float						mMaxAscent = 0.0f;
	float						mMaxDescent = 0.0f;
};

SdfText::TextureAtlas::TextureAtlas()
{
}

SdfText::TextureAtlas::TextureAtlas( FT_Face face, const SdfText::Format &format, const std::vector<SdfText::Font::Glyph> &glyphIndices )
	: mFace( face ), mSdfScale( format.getSdfScale() ), mSdfPadding( format.getSdfPadding() )
{
	const ivec2& tileSpacing = format.getSdfTileSpacing();

	// CW (TTF) vs CCW (OTF) - SDF needs to be inverted if font is OTF
	bool invertSdf = ( std::string( "OTTO" ) ==  std::string( reinterpret_cast<const char *>( face->stream->base ) ) );

	// Build glyph information that will be needed later
	for( const auto& glyphIndex : glyphIndices ) {
		// Glyph bounds, 
		msdfgen::Shape shape;
		if( msdfgen::loadGlyph( shape, face, glyphIndex ) ) {
			double l, b, r, t;
			l = b = r = t = 0.0;
			shape.bounds( l, b, r, t );
			// Glyph bounds
			Rectf bounds = Rectf( 
				static_cast<float>( l ), 
				static_cast<float>( b ), 
				static_cast<float>( r ), 
				static_cast<float>( t ) );
			mGlyphInfo[glyphIndex].mOriginOffset = vec2( l, b );
			mGlyphInfo[glyphIndex].mSize = vec2( r - l, t - b );
			// Max glyph size
			mMaxGlyphSize.x = std::max( mMaxGlyphSize.x, bounds.getWidth() );
			mMaxGlyphSize.y = std::max( mMaxGlyphSize.y, bounds.getHeight() );
			// Max ascent, descent
			mMaxAscent = std::max( mMaxAscent, static_cast<float>( t ) );
			mMaxDescent = std::max( mMaxAscent, static_cast<float>( std::fabs( b ) ) );
			//CI_LOG_I( (char)ch << " : " << mGlyphInfo[glyphIndex].mOriginOffset );
		}	
	}

	// Determine render bitmap size
	mSdfBitmapSize = SdfText::TextureAtlas::calculateSdfBitmapSize( mSdfScale, mSdfPadding, mMaxGlyphSize );
	// Determine glyph counts (per texture atlas)
	const size_t numGlyphColumns   = ( format.getTextureWidth()  / ( mSdfBitmapSize.x + tileSpacing.x ) );
	const size_t numGlyphRows      = ( format.getTextureHeight() / ( mSdfBitmapSize.y + tileSpacing.y ) );
	const size_t numGlyphsPerAtlas = numGlyphColumns * numGlyphRows;
	
	// Render position for each glyph
	struct RenderGlyph {
		uint32_t glyphIndex;
		ivec2    position;
	};

	std::vector<std::vector<RenderGlyph>> renderAtlases;

	// Build the atlases
	size_t curRenderIndex = 0;
	ivec2 curRenderPos = ivec2( 0 );
	std::vector<RenderGlyph> curRenderGlyphs;
	for( std::vector<SdfText::Font::Glyph>::const_iterator glyphIndexIt = glyphIndices.begin(); glyphIndexIt != glyphIndices.end() ;  ) {
		// Build render glyph
		RenderGlyph renderGlyph;
		renderGlyph.glyphIndex = *glyphIndexIt;
		renderGlyph.position.x = curRenderPos.x;
		renderGlyph.position.y = curRenderPos.y;
		
		// Add to render atlas
		curRenderGlyphs.push_back( renderGlyph );

		// Increment index
		++curRenderIndex;
		// Increment glyph index iterator
		++glyphIndexIt;
		// Advance horizontal position
		curRenderPos.x += mSdfBitmapSize.x;
		curRenderPos.x += tileSpacing.x;
		// Move to next row if needed
		if( 0 == ( curRenderIndex % numGlyphColumns ) ) {
			curRenderPos.x = 0;
			curRenderPos.y += mSdfBitmapSize.y;
			curRenderPos.y += tileSpacing.y;
		}

		if( ( numGlyphsPerAtlas == curRenderIndex ) || ( glyphIndices.end() == glyphIndexIt ) ) {
			// Copy current atlas
			renderAtlases.push_back( curRenderGlyphs );
			// Reset values
			curRenderIndex = 0;
			curRenderPos = ivec2( 0 );
			curRenderGlyphs.clear();
		}
	}

	// Surface
	Surface8u surface( format.getTextureWidth(), format.getTextureHeight(), false );
	ip::fill( &surface, Color8u( 0, 0, 0 ) );
	uint8_t *surfaceData   = surface.getData();
	size_t surfacePixelInc = surface.getPixelInc();
	size_t surfaceRowBytes = surface.getRowBytes();

	// Render the atlases
	const double sdfRange = static_cast<double>( format.getSdfRange() );
	const double sdfAngle = static_cast<double>( format.getSdfAngle() );
	msdfgen::Bitmap<msdfgen::FloatRGB> sdfBitmap( mSdfBitmapSize.x, mSdfBitmapSize.y );
	uint32_t currentTextureIndex = 0;
	for( size_t atlasIndex = 0; atlasIndex < renderAtlases.size(); ++atlasIndex ) {
		const auto& renderGlyphs = renderAtlases[atlasIndex];
		// Render atlas
		for( const auto& renderGlyph : renderGlyphs ) {
			msdfgen::Shape shape;
			if( msdfgen::loadGlyph( shape, face, renderGlyph.glyphIndex ) ) {
				shape.inverseYAxis = true;
				shape.normalize();	
				
				// Edge color
				msdfgen::edgeColoringSimple( shape, sdfAngle );

				// Generate SDF
				vec2 originOffset = mGlyphInfo[renderGlyph.glyphIndex].mOriginOffset;
				float tx = mSdfPadding.x;
				float ty = std::fabs( originOffset.y ) + mSdfPadding.y;
				// mSdfScale will get applied to <tx, ty> by msdfgen
				msdfgen::generateMSDF( sdfBitmap, shape, sdfRange, msdfgen::Vector2( mSdfScale.x, mSdfScale.y ), msdfgen::Vector2( tx, ty ) );

				// Invert the SDF if needed, but only for glyphs that have contours to render. 
				// Glyph without contours will produce and blank bitmap, inverting this produces
				// a solid block. Which is undesirable.
				if( invertSdf && ( ! shape.contours.empty() ) ) {
					for( int y = 0; y < sdfBitmap.height(); ++y ) {
						for( int x = 0; x < sdfBitmap.width(); ++x ) {
							sdfBitmap( x, y ).r = 1.0f - sdfBitmap( x, y ).r;
							sdfBitmap( x, y ).g = 1.0f - sdfBitmap( x, y ).g;
							sdfBitmap( x, y ).b = 1.0f - sdfBitmap( x, y ).b;
						}
					}
				}

				// Copy bitmap
				size_t dstOffset = ( renderGlyph.position.y * surfaceRowBytes ) + ( renderGlyph.position.x * surfacePixelInc );
				uint8_t *dst = surfaceData + dstOffset;
				for( int n = 0; n < mSdfBitmapSize.y; ++n ) {
					Color8u *dstPixel = reinterpret_cast<Color8u *>( dst );
					for( int m = 0; m < mSdfBitmapSize.x; ++m ) {
						msdfgen::FloatRGB &src = sdfBitmap( m, n );
						Color srcPixel = Color( src.r, src.g, src.b );
						*dstPixel = srcPixel;
						++dstPixel;
					}
					dst += surfaceRowBytes;
				}

				// Tex coords
				mGlyphInfo[renderGlyph.glyphIndex].mTextureIndex = currentTextureIndex;
				mGlyphInfo[renderGlyph.glyphIndex].mTexCoords = Area( 0, 0, mSdfBitmapSize.x, mSdfBitmapSize.y ) + renderGlyph.position;
			}
		}
		// Create texture
		gl::TextureRef tex = gl::Texture::create( surface );
		mTextures.push_back( tex );
		++currentTextureIndex;

		// Debug output
		//writeImage( "sdfText_" + std::to_string( atlasIndex ) + ".png", surface );

		// Reset
		ip::fill( &surface, Color8u( 0, 0, 0 ) );		
	}
}

SdfText::TextureAtlasRef SdfText::TextureAtlas::create( FT_Face face, const SdfText::Format &format, const std::vector<SdfText::Font::Glyph> &glyphIndices )
{
	SdfText::TextureAtlasRef result = SdfText::TextureAtlasRef( new SdfText::TextureAtlas( face, format, glyphIndices ) );
	return result;
}

cinder::ivec2 SdfText::TextureAtlas::calculateSdfBitmapSize( const vec2 &sdfScale, const ivec2& sdfPadding, const vec2 &maxGlyphSize )
{
	ivec2 result = ivec2( ( sdfScale * ( maxGlyphSize + ( 2.0f * vec2( sdfPadding ) ) ) ) + vec2( 0.5f ) );
	return result;
}

// =================================================================================================
// SdfTextManager
// =================================================================================================
class SdfTextManager {
public:
	~SdfTextManager();

	static SdfTextManager			*instance();

	FT_Library						getLibrary() const { return mLibrary; }

	const std::vector<std::string>&	getNames( bool forceRefresh );
	SdfText::Font					getDefault() const;

	struct FontInfo {
		std::string 	key;
		std::string 	name;
		fs::path 		path;
		FontInfo() {}
		FontInfo( const std::string& aKey, const std::string& aName, const fs::path& aPath ) 
			: key( aKey ), name( aName ), path( aPath ) {}
	};

	FontInfo 						getFontInfo( const std::string& fontName ) const;

private:
	SdfTextManager();

	static SdfTextManager			*sInstance;

	FT_Library						mLibrary = nullptr;
	bool							mFontsEnumerated = false;
	std::vector<std::string>		mFontNames;
	std::vector<FontInfo>			mFontInfos;
	std::set<FT_Face>				mTrackedFaces;
	mutable SdfText::Font			mDefault;

	SdfText::TextureAtlas::AtlasCacher		mTrackedTextureAtlases;

	void							acquireFontNamesAndPaths();
	void							faceCreated( FT_Face face );
	void							faceDestroyed( FT_Face face );

	SdfText::TextureAtlasRef		getTextureAtlas( FT_Face face, const SdfText::Format &format, const std::string &utf8Chars, const std::vector<SdfText::Font::Glyph> &glyphIndices );

	friend class SdfText;
	friend class SdfText::FontData;
	friend bool SdfTextFontManager_destroyStaticInstance();
};

// =================================================================================================
// SdfTextBox
// =================================================================================================
class SdfTextBox {
public:
	enum { GROW = 0 };
	
	SdfTextBox( const SdfText *sdfText ) : mSdfText( sdfText ), mAlign( SdfText::LEFT ), mSize( GROW, GROW ), mInvalid( true ), mLigate( true ) {}

	SdfTextBox&				size( ivec2 sz ) { setSize( sz ); return *this; }
	SdfTextBox&				size( int width, int height ) { setSize( ivec2( width, height ) ); return *this; }
	ivec2					getSize() const { return mSize; }
	void					setSize( ivec2 sz ) { mSize = sz; mInvalid = true; }

	SdfTextBox&				text( const std::string &t ) { setText( t ); return *this; }
	const std::string&		getText() const { return mText; }
	void					setText( const std::string &t ) { mText = t; mInvalid = true; }
	void					appendText( const std::string &t ) { mText += t; mInvalid = true; }

	SdfTextBox&				ligate( bool ligateText = true ) { setLigate( ligateText ); return *this; }
	bool					getLigate() const { return mLigate; }
	void					setLigate( bool ligateText ) { mLigate = ligateText; }

	SdfTextBox&				alignment( SdfText::Alignment align ) { setAlignment( align ); return *this; }
	SdfText::Alignment		getAlignment() const { return mAlign; }
	void					setAlignment( SdfText::Alignment align ) { mAlign = align; mInvalid = true; }

	std::vector<std::string>			calculateLineBreaks() const;
	SdfText::Font::GlyphMeasuresList	measureGlyphs( const SdfText::DrawOptions& drawOptions ) const;

private:
	const SdfText		*mSdfText = nullptr;
	SdfText::Alignment	mAlign;
	ivec2				mSize;
	std::string			mText;
	bool				mLigate;
	mutable bool		mInvalid;
};

// =================================================================================================
// SdfTexttManager Implementation
// =================================================================================================
SdfTextManager* SdfTextManager::sInstance = nullptr;

bool SdfTextFontManager_destroyStaticInstance() 
{
	if( nullptr != SdfTextManager::sInstance ) {
		delete SdfTextManager::sInstance;
		SdfTextManager::sInstance = nullptr;
	}
	return true;
}

SdfTextManager::SdfTextManager()
{
	FT_Error ftRes = FT_Init_FreeType( &mLibrary );
	if( FT_Err_Ok != ftRes ) {
		throw FontInvalidNameExc("Failed to initialize FreeType2");
	}

	acquireFontNamesAndPaths();
#if defined( CINDER_MSW )
	// Registry operations can be rejected by Windows so no fonts will be picked up 
	// on the initial scan. So we can multiple times.
	if( mFontInfos.empty() ) {
		for( int i = 0; i < 5; ++i ) {
			acquireFontNamesAndPaths();
			if( ! mFontInfos.empty() ) {
				break;
			}
			::Sleep( 10 );
		}
	}
#endif
}

SdfTextManager::~SdfTextManager()
{
	if( nullptr != mLibrary ) {
		for( auto& face : mTrackedFaces ) {
			FT_Done_Face( face );
		}

		FT_Done_FreeType( mLibrary );
	}

#if defined( CINDER_MAC )
#elif defined( CINDER_WINRT )
#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
#endif
}

SdfTextManager* SdfTextManager::instance()
{
	if( nullptr == SdfTextManager::sInstance ) {
		SdfTextManager::sInstance =  new SdfTextManager();
		if( nullptr != SdfTextManager::sInstance ) {
			ci::app::App::get()->getSignalShouldQuit().connect( SdfTextFontManager_destroyStaticInstance );
		}
	}
	
	return SdfTextManager::sInstance;
}

#if defined( CINDER_MAC )
void SdfTextManager::acquireFontNamesAndPaths()
{
	NSFontManager *nsFontManager = [NSFontManager sharedFontManager];
    NSArray *nsFontNames = [nsFontManager availableFonts];
    for( NSString *nsFontName in nsFontNames ) {
        std::string fontName = std::string( [nsFontName UTF8String] );
        mFontNames.push_back( fontName );
        
        CTFontDescriptorRef ctFont = CTFontDescriptorCreateWithNameAndSize( (__bridge CFStringRef)nsFontName, (CGFloat)24 );
        CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute( ctFont, kCTFontURLAttribute );
        NSString *nsFontPath = [NSString stringWithString:[(NSURL *)CFBridgingRelease(url) path]];
        std::string fontFilePath = std::string( [nsFontPath UTF8String] );
        
        if( fs::exists( fontFilePath ) ) {
			std::string fontKey = boost::to_lower_copy( fontName );
			auto it = std::find_if( std::begin( mFontInfos ), std::end( mFontInfos ),
				[fontKey]( const FontInfo& elem ) -> bool {
					return elem.key == fontKey;
				}
			);
			if( std::end( mFontInfos ) == it ) {
                // Build font info
                FontInfo fontInfo = FontInfo( fontKey, fontName, fontFilePath );
                mFontInfos.push_back( fontInfo );
                mFontNames.push_back( fontName );
            }
        }
    }
}
#elif defined( CINDER_COCOA_TOUCH )
void SdfTextManager::acquireFontNamesAndPaths()
{
    NSArray *nsFamilyNames = [UIFont familyNames];
    for( NSString *nsFamilyName in nsFamilyNames ) {
        NSArray *nsFontNames = [UIFont fontNamesForFamilyName:nsFamilyName];
        for( NSString *nsFontName in nsFontNames ) {
            std::string fontName = std::string( [nsFontName UTF8String] );
            mFontNames.push_back( fontName );
            
            CTFontDescriptorRef ctFont = CTFontDescriptorCreateWithNameAndSize( (__bridge CFStringRef)nsFontName, (CGFloat)24 );
            CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute( ctFont, kCTFontURLAttribute );
            NSString *nsFontPath = [NSString stringWithString:[(NSURL *)CFBridgingRelease(url) path]];
            std::string fontFilePath = std::string( [nsFontPath UTF8String] );
            
            if( fs::exists( fontFilePath ) ) {
                std::string fontKey = boost::to_lower_copy( fontName );
                auto it = std::find_if( std::begin( mFontInfos ), std::end( mFontInfos ),
                    [fontKey]( const FontInfo& elem ) -> bool {
                        return elem.key == fontKey;
                    }
                );
                if( std::end( mFontInfos ) == it ) {
                    // Build font info
                    FontInfo fontInfo = FontInfo( fontKey, fontName, fontFilePath );
                    mFontInfos.push_back( fontInfo );
                    mFontNames.push_back( fontName );
                }
            }
        }
    }
}
#elif defined( CINDER_MSW )
void SdfTextManager::acquireFontNamesAndPaths()
{
	static const LPWSTR kFontRegistryPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

	// Open Windows font registry key
	HKEY hKey = nullptr;
	LONG result = RegOpenKeyEx( HKEY_LOCAL_MACHINE, kFontRegistryPath, 0, KEY_READ, &hKey );
	if( ERROR_SUCCESS != result ) {
		return;
	}
	
	// Get info for registry key
	DWORD maxValueNameSize, maxValueDataSize;
	result = RegQueryInfoKey( hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0 );
	if( ERROR_SUCCESS != result ) {
		return;
	}

	DWORD valueIndex = 0;
	LPWSTR valueName = new WCHAR[maxValueNameSize];
	LPBYTE valueData = new BYTE[maxValueDataSize];
	DWORD valueNameSize, valueDataSize, valueType;

	// Enumerate registry keys
	do {
		valueNameSize = maxValueNameSize;
		valueDataSize = maxValueDataSize;

		// Clear the buffers
		std::memset( valueName, 0, maxValueNameSize*sizeof( WCHAR ) );
		std::memset( valueData, 0, maxValueDataSize*sizeof( BYTE ) );

		// Read registry key values
		result = RegEnumValue( hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize );
		++valueIndex;

		// Bail if read fails
		if( ( ERROR_SUCCESS != result ) || ( REG_SZ != valueType ) ) {
			continue;
		}

		// Build font info
		const std::string kTrueTypeTag = "(TrueType)";
		// Font name
		std::wstring wsFontName = std::wstring( valueName, valueNameSize );
		std::string fontName = ci::toUtf8( reinterpret_cast<const char16_t *>( wsFontName.c_str() ), wsFontName.length() * sizeof( char16_t ) );
		// Font file path
		//std::wstring wsFontFilePath = std::wstring( reinterpret_cast<LPWSTR>( valueData ), valueDataSize );
		std::wstring wsFontFilePath;
		for( size_t i = 0; i < valueDataSize; ++i ) {
			WCHAR ch = reinterpret_cast<LPWSTR>( valueData )[i];
			if( 0 == ch ) {
				break;
			}
			wsFontFilePath.push_back( ch );		
		}
		std::string fontFilePath = ci::toUtf8( reinterpret_cast<const char16_t *>( wsFontFilePath.c_str() ), wsFontFilePath.length() * sizeof( char16_t ) );
		// Process True Type font
		if( std::string::npos != fontName.find( kTrueTypeTag ) ) {
			boost::replace_all( fontName, kTrueTypeTag, "" );
			boost::trim( fontName );
			std::string fontKey = boost::to_lower_copy( fontName );
			auto it = std::find_if( std::begin( mFontInfos ), std::end( mFontInfos ),
				[fontKey]( const FontInfo& elem ) -> bool {
					return elem.key == fontKey;
				}
			);
			if( std::end( mFontInfos ) == it ) {
				fontFilePath = "C:\\Windows\\Fonts\\" + fontFilePath;
				if( fs::exists( fontFilePath ) ) {
					// Build font info
					FontInfo fontInfo = FontInfo( fontKey, fontName, fontFilePath );
					mFontInfos.push_back( fontInfo );
					mFontNames.push_back( fontName );
				}
			}
		}
	} 
	while( ERROR_NO_MORE_ITEMS != result  );

	delete [] valueName;
	delete [] valueData;
}
#elif defined( CINDER_WINRT ) 
void SdfTextManager::acquireFontNamesAndPaths()
{
}
#elif defined( CINDER_ANDROID )
void SdfTextManager::acquireFontNamesAndPaths()
{
	fs::path systemFontDir = "/system/fonts";
	if( fs::exists( systemFontDir ) && fs::is_directory( systemFontDir ) ) {
		fs::directory_iterator end_iter;
		for( fs::directory_iterator dir_iter( systemFontDir ) ; dir_iter != end_iter ; ++dir_iter ) {
			if( fs::is_regular_file( dir_iter->status() ) ) {
				fs::path fontPath = dir_iter->path();

				FT_Face tmpFace;
				FT_Error error = FT_New_Face( mLibrary, fontPath.string().c_str(), 0, &tmpFace );
				if( error ) {
					continue;
				}

				std::string fontName = ci::linux::ftutil::GetFontName( tmpFace, fontPath.stem().string() );
				std::string keyName = fontName;
				std::transform( keyName.begin(), keyName.end(), keyName.begin(), [](char c) -> char { return (c >= 'A' && c <='Z') ? (c + 32) : c; } );
				mFontInfos.push_back( FontInfo( keyName, fontName, fontPath ) );

				const std::string regular = "regular";
				size_t startPos = keyName.find( regular );
				if( std::string::npos != startPos ) {
					keyName.replace( startPos, regular.length(), "" );
					mFontInfos.push_back( FontInfo( keyName, fontName, fontPath ) );
				} 	

				FT_Done_Face( tmpFace );
			}
		}
	}
}
#elif defined( CINDER_LINUX )
void SdfTextManager::acquireFontNamesAndPaths()
{
	if( ::FcInit() ) {
		::FcPattern   *pat = ::FcPatternCreate();
		::FcObjectSet *os  = ::FcObjectSetBuild( FC_FILE, FC_FAMILY, FC_STYLE, (char *)0 );
		::FcFontSet   *fs  = ::FcFontList (0, pat, os);
	
		for( size_t i = 0; i < fs->nfont; ++i ) {
			//::FcPattern *font = fs->fonts[i];
			//::FcChar8 *family = nullptr;
			//if( ::FcPatternGetString( font, FC_FAMILY, 0, &family ) == FcResultMatch ) {					
			//	std::cout << "Found font family: " << family << std::endl;
			//	//string fontName = std::string( (const char*)family );
			//	//mFontNames.push_back( fontName );
			//}

			::FcPattern *fcFont = fs->fonts[i];
			::FcChar8 *fcFileName = nullptr;
			if( ::FcResultMatch == ::FcPatternGetString( fcFont, FC_FILE, 0, &fcFileName ) ) {
				std::string fontFilePath = std::string( (const char*)fcFileName );
				
				// Skip anything that isn't ttf or otf
				std::string lcfn = boost::to_lower_copy( fontFilePath );
				boost::trim( lcfn );
				if( ! ( boost::ends_with( lcfn, ".ttf" ) || boost::ends_with( lcfn, ".otf" ) ) ) {
					continue;
				}

				::FcChar8 *fcFamily = nullptr;
				::FcChar8 *fcStyle = nullptr;
				::FcResult fcFamilyRes = ::FcPatternGetString( fcFont, FC_FAMILY, 0, &fcFamily );
				::FcResult fcStyleRes = ::FcPatternGetString( fcFont, FC_STYLE, 0, &fcStyle );
				if( ( ::FcResultMatch != fcFamilyRes ) || ( ::FcResultMatch != fcStyleRes ) ) {
					continue;
				}

				std::string family = std::string( (const char*)fcFamily );
				std::string style = std::string( (const char*)fcStyle );
				std::string fontName = family + ( style.empty() ? "" : ( " " + style ) );
				
				std::string fontKey = boost::to_lower_copy( fontName );
				auto it = std::find_if( std::begin( mFontInfos ), std::end( mFontInfos ),
					[fontKey]( const FontInfo& elem ) -> bool {
						return elem.key == fontKey;
					}
				);
				if( std::end( mFontInfos ) == it ) {
					if( fs::exists( fontFilePath ) ) {
						// Build font info
						FontInfo fontInfo = FontInfo( fontKey, fontName, fontFilePath );
						mFontInfos.push_back( fontInfo );
						mFontNames.push_back( fontName );
					}
				}

				//std::cout << fcFamily << " " << fcStyle << " : " << fontFilePath << std::endl;

				/*
				DataSourceRef dataSource = ci::loadFile( fontFilePath );
				if( ! dataSource ) {
					throw FontLoadFailedExc( "Couldn't find file for " + aName );
				}

				mFileData = dataSource->getBuffer();
				FT_Error error = FT_New_Memory_Face(
					FontManager::instance()->mLibrary, 
					(FT_Byte*)mFileData->getData(), 
					mFileData->getSize(), 
					0, 
					&mFace
				);
				if( error ) {
					throw FontInvalidNameExc( "Failed to create a face for " + aName );
				}

				FT_Select_Charmap( mFace, FT_ENCODING_UNICODE );
				FT_Set_Char_Size( mFace, 0, (int)aSize * 64, 0, 72 );
				*/
			}			
		}

		::FcObjectSetDestroy( os );
		::FcPatternDestroy( pat );
		::FcFontSetDestroy( fs );

		::FcFini();
	}
}
#endif

void SdfTextManager::faceCreated( FT_Face face ) 
{
	mTrackedFaces.insert( face );
}

void SdfTextManager::faceDestroyed( FT_Face face ) 
{
	mTrackedFaces.erase( face );
}

SdfText::TextureAtlasRef SdfTextManager::getTextureAtlas( FT_Face face, const SdfText::Format &format, const std::string &utf8Chars, const std::vector<SdfText::Font::Glyph> &glyphIndices )
{
	std::u32string utf32Chars = ci::toUtf32( utf8Chars );
	// Add a space if needed
	if( std::string::npos == utf8Chars.find( ' ' ) ) {
		utf32Chars += ci::toUtf32( " " );
	}

	// Build the maps and information pieces that will be needed later
	vec2 maxGlyphSize = vec2( 0 );
	for( const auto& ch : utf32Chars ) {
		FT_UInt glyphIndex = FT_Get_Char_Index( face, static_cast<FT_ULong>( ch ) );
		// Glyph bounds, 
		msdfgen::Shape shape;
		if( msdfgen::loadGlyph( shape, face, glyphIndex ) ) {
			double l, b, r, t;
			l = b = r = t = 0.0;
			shape.bounds( l, b, r, t );
			// Glyph bounds
			Rectf bounds = Rectf( 
				static_cast<float>( l ), 
				static_cast<float>( b ), 
				static_cast<float>( r ), 
				static_cast<float>( t ) );
			// Max glyph size
			maxGlyphSize.x = std::max( maxGlyphSize.x, bounds.getWidth() );
			maxGlyphSize.y = std::max( maxGlyphSize.y, bounds.getHeight() );
		}	
	}
	
	SdfText::TextureAtlas::CacheKey key;
	key.mFamilyName = std::string( face->family_name );
	key.mStyleName = std::string( face->style_name );
	key.mUtf8Chars = utf8Chars;
	key.mTextureSize = format.getTextureSize();
	key.mSdfBitmapSize = SdfText::TextureAtlas::calculateSdfBitmapSize( format.getSdfScale(), format.getSdfPadding(), maxGlyphSize );

	// Result
	SdfText::TextureAtlasRef result;
	// Look for the texture atlas 
	auto it = std::find_if( std::begin( mTrackedTextureAtlases ), std::end( mTrackedTextureAtlases ),
		[key]( const std::pair<SdfText::TextureAtlas::CacheKey, SdfText::TextureAtlasRef>& elem ) -> bool {
			return elem.first == key;
		}
	);
	// Use the texture atlas if a matching one is found
	if( mTrackedTextureAtlases.end() != it ) {
		result = it->second;
	}
	// ...otherwise build a new one
	else {
		result = SdfText::TextureAtlas::create( face, format, glyphIndices );
		mTrackedTextureAtlases.push_back( std::make_pair( key, result ) );
	}

	return result;
}

SdfTextManager::FontInfo SdfTextManager::getFontInfo( const std::string& fontName ) const
{
	SdfTextManager::FontInfo result;

#if defined( CINDER_MAC )
#elif defined( CINDER_MSW ) || defined( CINDER_WINRT )
	result.key  = "arial";
	result.name = "Arial";
	result.path = "C:\\Windows\\Fonts\\arial.ttf";
#elif defined( CINDER_ANDROID )
	result.key  = "roboto regular";
	result.name = "Roboto Regular";
	result.path = "/system/fonts/Roboto-Regular.ttf";
#elif defined( CINDER_LINUX )	
#endif

	std::string lcfn = boost::to_lower_copy( fontName );
	boost::trim( lcfn );

	auto it = std::find_if( std::begin( mFontInfos ), std::end( mFontInfos ),
		[lcfn]( const SdfTextManager::FontInfo &elem ) -> bool {
			return elem.key == lcfn;
		}
	);

	if( std::end( mFontInfos ) != it ) {
		result = *it;
	}
	else {
		std::vector<std::string> tokens = ci::split( lcfn, ' ' );
		float highScore = 0.0f;
		for( const auto& fontInfos : mFontInfos ) {
			int hits = 0;
			for( const auto& tok : tokens ) {
				if( std::string::npos != fontInfos.key.find( tok ) ) {
					hits += static_cast<int>( tok.size() );	
				}
			}

			if( hits > 0 ) {
				std::vector<std::string> keyTokens = ci::split( fontInfos.key, ' ' );
				float keyScore = ( keyTokens.size() == tokens.size() ) ? 0.25f : 0.0f;
				float hitScore = static_cast<float>( hits ) / static_cast<float>( fontInfos.key.length() - ( keyTokens.size() - 1 ) );
				hitScore = 0.75f * std::min( hitScore, 1.0f );
				float totalScore = keyScore + hitScore;
				if( totalScore > highScore ) {
					highScore = totalScore;
					result = fontInfos;
				}
			}
		}
	}

	return result;
}

const std::vector<std::string>& SdfTextManager::getNames( bool forceRefresh )
{
	if( ( ! mFontsEnumerated ) || forceRefresh ) {
		mFontInfos.clear();
		mFontNames.clear();

		acquireFontNamesAndPaths();
#if defined( CINDER_MSW )
		// Registry operations can be rejected by Windows so no fonts will be picked up 
		// on the initial scan. So we can multiple times.
		if( mFontInfos.empty() ) {
			for( int i = 0; i < 5; ++i ) {
				acquireFontNamesAndPaths();
				if( ! mFontInfos.empty() ) {
					break;
				}
				::Sleep( 10 );
			}
		}
#endif        

		mFontsEnumerated = true;
	}

/*
	if( ( ! mFontsEnumerated ) || forceRefresh ) {
		mFontNames.clear();
#if defined( CINDER_MAC )
		NSArray *fontNames = [nsFontManager availableFonts];
		for( NSString *fontName in fontNames ) {
			mFontNames.push_back( string( [fontName UTF8String] ) );
		}
#elif defined( CINDER_COCOA_TOUCH )
		NSArray *familyNames = [UIFont familyNames];
		for( NSString *familyName in familyNames ) {
			NSArray *fontNames = [UIFont fontNamesForFamilyName:familyName];
			for( NSString *fontName in fontNames ) {
				mFontNames.push_back( string( [fontName UTF8String] ) );
			}
		}
#elif defined( CINDER_MSW )
	acquireFontPaths();
	// Registry operations can be rejected by Windows so no fonts will be picked up 
	// on the initial scan. So we can multiple times.
	if( mFontInfos.empty() ) {
		for( int i = 0; i < 5; ++i ) {
			acquireFontPaths();
			if( ! mFontInfos.empty() ) {
				break;
			}
			::Sleep( 10 );
		}
	}
#elif defined( CINDER_WINRT )
		Platform::Array<Platform::String^>^ fontNames = FontEnumeration::FontEnumerator().ListSystemFonts();
		for( unsigned i = 0; i < fontNames->Length; ++i ) {
			const wchar_t *start = fontNames[i]->Begin();
			const wchar_t *end = fontNames[i]->End();
			mFontNames.push_back(std::string(start, end));
		}
#elif defined( CINDER_ANDROID )
		std::set<std::string> uniqueNames;
		for( const auto& fontInfos : mFontInfos ) {
			uniqueNames.insert( fontInfos.name );
		}

		for( const auto& name : uniqueNames ) {
			mFontNames.push_back( name );
		}
#elif defined( CINDER_LINUX )
		if( ::FcInit() ) {
			::FcPattern   *pat = ::FcPatternCreate();
			::FcObjectSet *os  = ::FcObjectSetBuild( FC_FILE, FC_FAMILY, FC_STYLE, (char *)0 );
			::FcFontSet   *fs  = ::FcFontList (0, pat, os);
		
			for( size_t i = 0; i < fs->nfont; ++i ) {
				::FcPattern *font = fs->fonts[i];
				::FcChar8 *family = nullptr;
				if( ::FcPatternGetString( font, FC_FAMILY, 0, &family ) == FcResultMatch ) 
				{					
					string fontName = std::string( (const char*)family );
					mFontNames.push_back( fontName );
				}
			}

			::FcObjectSetDestroy( os );
			::FcPatternDestroy( pat );
			::FcFontSetDestroy( fs );

			::FcFini();
		}
#endif
		mFontsEnumerated = true;
	}
*/	

	return mFontNames;
}

SdfText::Font SdfTextManager::getDefault() const
{
	if( ! mDefault ) {
#if defined( CINDER_COCOA )        
		mDefault = SdfText::Font( "Helvetica", 32.0f );
#elif defined( CINDER_MSW ) || defined( CINDER_WINRT )
		mDefault = SdfText::Font( "Arial", 32.0f );
#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
		mDefault = SdfText::Font( "Roboto", 32.0f );
#endif
	}

	return mDefault;
}

// =================================================================================================
// SdfTextBox Implementation
// =================================================================================================
struct LineProcessor 
{
	LineProcessor( std::vector<std::string> *strings ) : mStrings( strings ) {}
	void operator()( const char *line, size_t len ) const { mStrings->push_back( std::string( line, len ) ); }
	mutable std::vector<std::string> *mStrings = nullptr;
};

struct LineMeasure 
{
	LineMeasure( float maxWidth, const SdfText::Font::GlyphMetricsMap &cachedGlyphMetrics, const SdfText::Font::CharToGlyphMap& charToGlyphMap ) 
		: mMaxWidth( maxWidth ), mCachedGlyphMetrics( cachedGlyphMetrics ), mCharToGlyphMap( charToGlyphMap ) {}

	bool operator()( const char *line, size_t len ) const {
		if( mMaxWidth >= MAX_SIZE ) {
			// too big anyway so just return true
			return true;
		}

		SdfText::Font::Glyph subGlyphIndex = 0;
		// Use space for glyph substitution 
		{
			auto glyphIndexIt = mCharToGlyphMap.find( static_cast<uint32_t>( ' ' ) );
			if( mCharToGlyphMap.end() != glyphIndexIt ) {
				subGlyphIndex = glyphIndexIt->second;
			}
		}

		std::u32string utf32Chars = ci::toUtf32( std::string( line, len ) );
		float measuredWidth = 0;
		vec2 pen = { 0, 0 };
		for( const auto& ch : utf32Chars ) {
			auto glyphIndexIt = mCharToGlyphMap.find( static_cast<uint32_t>( ch ) );
			if( mCharToGlyphMap.end() == glyphIndexIt ) {
				continue;
			}

			SdfText::Font::Glyph glyphIndex = glyphIndexIt->second;
			auto glyphMetricIt = mCachedGlyphMetrics.find( glyphIndex );
			if( mCachedGlyphMetrics.end() == glyphMetricIt ) {
				continue;
			}

			const vec2& advance = glyphMetricIt->second.advance;		
			pen.x += advance.x;
			pen.y += advance.y;
			measuredWidth = pen.x;
		}

		bool result = ( measuredWidth <= mMaxWidth );
		return result;
	}

	float									mMaxWidth = 0;
	const SdfText::Font::GlyphMetricsMap	&mCachedGlyphMetrics;
	const SdfText::Font::CharToGlyphMap		&mCharToGlyphMap;
};

std::vector<std::string> SdfTextBox::calculateLineBreaks() const
{
	const auto& charToGlyph = mSdfText->getCharToGlyph();
	const auto& glyphMetrics = mSdfText->getGlyphMetrics();

	std::vector<std::string> result;
	std::function<void(const char *,size_t)> lineFn = LineProcessor( &result );		
	lineBreakUtf8( mText.c_str(), LineMeasure( ( mSize.x > 0 ) ? static_cast<float>( mSize.x ) : MAX_SIZE, glyphMetrics, charToGlyph ), lineFn );
	return result;
}

SdfText::Font::GlyphMeasuresList SdfTextBox::measureGlyphs( const SdfText::DrawOptions& drawOptions ) const
{
	SdfText::Font::GlyphMeasuresList result;

	if( mText.empty() ) {
		return result;
	}

	const auto& font = mSdfText->getFont();
	const float fontSizeScale = font.getSize() / 32.0f;
	const float ascent        = font.getAscent();
	const float descent       = font.getDescent();
	const float leading       = drawOptions.getLeading();
	const float drawScale	  = drawOptions.getScale();
	const auto  align         = drawOptions.getAlignment();
	const float lineHeight    = fontSizeScale * drawScale * ( ascent + descent + leading );

	// Calculate the line breaks
	std::vector<std::string> mLines = calculateLineBreaks();
	if( mLines.empty() ) {
		return result;
	}

	// Build measures
	const auto& charToGlyph = mSdfText->getCharToGlyph();
	const auto& glyphMetrics = mSdfText->getGlyphMetrics();
	std::u32string utf32Chars, nextUtf32Chars;
	float curY = 0;

	for( std::vector<std::string>::const_iterator lineIt = mLines.begin(); lineIt != mLines.end(); ++lineIt ) {
		// Fetch current line and prefetch next. This way we can look ahead.
		if( nextUtf32Chars.empty() ) {
			utf32Chars = ci::toUtf32( boost::algorithm::trim_right_copy( *lineIt ) );
		}
		else {
			std::swap( utf32Chars, nextUtf32Chars );
		}

		if( ( lineIt + 1 ) != mLines.end() ) {
			nextUtf32Chars = ci::toUtf32( boost::algorithm::trim_right_copy( *( lineIt + 1 ) ) );
		}
		else {
			nextUtf32Chars.clear();
		}

		// Layout current line of text.
		size_t               index = result.size();
		size_t               spaceCount = 0;
		size_t               glyphCount = 0;
		SdfText::Font::Glyph spaceIndex = ~0;
		SdfText::Font::Glyph glyphIndex = ~0;
		vec2                 advance = vec2( 0 );
		vec2                 adjust = vec2( 0 );

		vec2 pen = { 0, 0 };
		for( const auto& ch : utf32Chars ) {
			auto glyphIndexIt = charToGlyph.find( static_cast<uint32_t>( ch ) );
			if( charToGlyph.end() == glyphIndexIt ) {
				continue;
			}
			 
			glyphIndex = glyphIndexIt->second;
			auto glyphMetricIt = glyphMetrics.find( glyphIndex );
			if( glyphMetrics.end() == glyphMetricIt ) {
				continue;
			}

			advance = glyphMetricIt->second.advance;
			adjust = advance - glyphMetricIt->second.maximum;

			glyphCount++;
			if( ch == 32 ) {
				spaceCount++;
				spaceIndex = glyphIndex;
			}

			float xPos = pen.x;
			result.push_back( std::make_pair( (uint32_t)glyphIndex, vec2( xPos, curY ) ) );

			pen += advance;
		}

		// Apply alignment as a post-process.
		bool aligned = false;
		if( drawOptions.getJustify() ) {
			const bool isLastLine = nextUtf32Chars.empty();
			if( spaceCount > 0 && !isLastLine ) {
				float space = ( mSize.x - ( pen.x + advance.x - adjust.x ) );
				float offset = 0.0f;
				for( size_t i = index; i < result.size(); ++i ) {
					result[i].second.x += offset;
					// 75% of the extra spacing comes from adjusting every character.
					offset += ( 0.75f * space ) / glyphCount;
					if( result[i].first == spaceIndex ) {
						// 25% of the extra spacing comes from adjusting white space characters only.
						offset += ( 0.25f * space ) / spaceCount;
					}
				}
				aligned = true;
			}
		}

		if( ! aligned ){
			switch( align ) {
				case SdfText::LEFT:
				break;
				case SdfText::CENTER: {
					float offset = ( mSize.x - ( pen.x + advance.x - adjust.x ) ) * 0.5f;
					if( offset > 0.0f ) {
						for( size_t i = index; i < result.size(); ++i )
							result[i].second.x += offset;
					}
				}
				break;
				case SdfText::RIGHT: {
					float offset = ( mSize.x - ( pen.x + advance.x - adjust.x ) );
					if( offset > 0.0f ) {
						for( size_t i = index; i < result.size(); ++i )
							result[i].second.x += offset;
					}
				}
				break;
			}
		}

		curY += lineHeight; 
	}

	return result;
}

// =================================================================================================
// SdfText::FontData
// =================================================================================================
class SdfText::FontData {
public:
	FontData( const ci::DataSourceRef &dataSource ) {
		if(! dataSource) {
			return;
		}

		mFileData = dataSource->getBuffer();
		if( ! mFileData ) {
			return;
		}

		auto fontManager = SdfTextManager::instance();
		if( nullptr != fontManager ) {
			FT_Error ftRes = FT_New_Memory_Face(
				fontManager->getLibrary(),
				reinterpret_cast<FT_Byte*>( mFileData->getData() ),
				static_cast<FT_Long>( mFileData->getSize() ),
				0,
				&mFace
			);

			if( FT_Err_Ok != ftRes ) {
				throw std::runtime_error("Failed to load font data");
			}

			fontManager->faceCreated( mFace );
		}
	}

	virtual ~FontData() {
		auto fontManager = SdfTextManager::instance();
		if( ( nullptr != fontManager ) && ( nullptr != mFace ) ) {
			fontManager->faceDestroyed( mFace );
		}
	}

	static SdfText::FontDataRef create( const ci::DataSourceRef &dataSource ) {
		SdfText::FontDataRef result = SdfText::FontDataRef( new SdfText::FontData( dataSource ) );
		return result;
	}

	FT_Face	getFace() const {
		return mFace;
	}

private:
	ci::BufferRef	mFileData;
	FT_Face			mFace = nullptr;
};

// =================================================================================================
// SdfText::Font
// =================================================================================================
SdfText::Font::Font( const std::string &name, float size )
	: mName( name ), mSize( size )
{
	auto fontManager = SdfTextManager::instance();
	if( nullptr != fontManager ) {
		SdfTextManager::FontInfo info = fontManager->getFontInfo( name );
		if( ! ci::fs::exists( info.path ) ) {
			throw std::runtime_error( info.path.string() + " does not exist" );
		}

		auto dataSource = ci::loadFile( info.path );
		loadFontData( dataSource );
	}
}

SdfText::Font::Font( DataSourceRef dataSource, float size )
	: mSize( size )
{
	if( dataSource->isFilePath() ) {
		auto fontDataSource = ci::loadFile( dataSource->getFilePath() );
		loadFontData( fontDataSource );
	}
	else {
		loadFontData( dataSource );
	}
}

SdfText::Font::~Font()
{
}

void SdfText::Font::loadFontData( const ci::DataSourceRef &dataSource )
{
	mData = SdfText::FontData::create( dataSource );
	FT_Select_Charmap( mData->getFace(), FT_ENCODING_UNICODE );

	FT_F26Dot6 finalSize = static_cast<FT_F26Dot6>( mSize * 64.0f );
	FT_Set_Char_Size( mData->getFace(), 0, finalSize , 0, 72 );

	// Extract the name if needed
	if( mName.empty() ) {
		FT_SfntName sn = {};
		if( FT_Err_Ok == FT_Get_Sfnt_Name( mData->getFace(), TT_NAME_ID_FULL_NAME, &sn ) ) {
			// If full name isn't available use family and style name
			if( sn.string_len > 0  && ( 0 == sn.string[0] ) ) {
				// Fallback to this name
				mName = "(Unknown)";
				std::string familyName = mData->getFace()->family_name;
				std::string styleName = mData->getFace()->style_name;
				if( ! familyName.empty() ) {
					mName = familyName;
					if( ! styleName.empty() ) {
						styleName += ( " " + styleName );
					}
				}
			}
			else {
				mName = std::string( reinterpret_cast<const char *>( sn.string ), sn.string_len );
			}
		}
	}

	// Units per EM
	mUnitsPerEm = mData->getFace()->units_per_EM;

	// Glyph scale
	float glyphScale = 2048.0f / static_cast<float>( mUnitsPerEm );

	// Height
	mHeight = glyphScale * ( mData->getFace()->height / 64.0f );

	// Leading
	mLeading = glyphScale * ( mData->getFace()->height - ( std::abs( mData->getFace()->ascender ) + std::abs( mData->getFace()->descender ) ) ) / 64.0f;

	// Ascent
	mAscent = glyphScale * std::fabs( mData->getFace()->ascender / 64.0f );

	// Descent
	mDescent = glyphScale * std::fabs( mData->getFace()->descender / 64.0f );
}

SdfText::Font::Glyph SdfText::Font::getGlyphIndex( size_t idx ) const
{
	return static_cast<SdfText::Font::Glyph>( idx );
}

SdfText::Font::Glyph SdfText::Font::getGlyphChar( char utf8Char ) const
{
	FT_UInt glyphIndex = FT_Get_Char_Index( mData->getFace(), static_cast<FT_ULong>( utf8Char ) );
	return static_cast<SdfText::Font::Glyph>( glyphIndex );
}

std::vector<SdfText::Font::Glyph> SdfText::Font::getGlyphs( const std::string &utf8Chars ) const
{
	std::vector<SdfText::Font::Glyph> result;
	// Convert to UTF32
	std::u32string utf32Chars = ci::toUtf32( utf8Chars );
	// Build the maps and information pieces that will be needed later
	for( const auto& ch : utf32Chars ) {
		FT_UInt glyphIndex = FT_Get_Char_Index( mData->getFace(), static_cast<FT_ULong>( ch ) );
		result.push_back( static_cast<SdfText::Font::Glyph>( glyphIndex ) );
	}
	return result;
}

FT_Face SdfText::Font::getFace() const
{
	return mData->getFace();
}

const std::vector<std::string>& SdfText::Font::getNames( bool forceRefresh )
{
	return SdfTextManager::instance()->getNames( forceRefresh );
}

SdfText::Font SdfText::Font::getDefault()
{
	return SdfTextManager::instance()->getDefault();
}

// =================================================================================================
// SdfText
// =================================================================================================
SdfText::SdfText( const SdfText::Font &font, const Format &format, const std::string &utf8Chars, bool generateSdf )
	: mFont( font ), mFormat( format )
{
	if( generateSdf ) {
		FT_Face face = font.getFace();
		if( nullptr == face ) {
			throw std::runtime_error( "null font face" );
		}

		// Convert characters from UTF8 to UTF32
		std::u32string utf32Chars = ci::toUtf32( utf8Chars );
		// Add a space if needed
		if( std::string::npos == utf8Chars.find( ' ' ) ) {
			utf32Chars += ci::toUtf32( " " );
		}

		// Build char/glyph maps
		std::vector<SdfText::Font::Glyph> glyphIndices;
		for( const auto &ch : utf32Chars ) {
			// Lookup glyph index based on char
			SdfText::Font::Glyph glyphIndex = static_cast<SdfText::Font::Glyph>( FT_Get_Char_Index( face, static_cast<FT_ULong>( ch ) ) );

			// Unique glyph
			auto it = std::find_if( std::begin( glyphIndices ), std::end( glyphIndices ),
				[glyphIndex]( const SdfText::Font::Glyph &elem ) -> bool {
					return ( elem == glyphIndex );
				}
			);
			if( std::end( glyphIndices ) == it ) {
				glyphIndices.push_back( glyphIndex );
			}

			// Character to glyph index and vice versa
			mCharToGlyph[static_cast<SdfText::Font::Char>( ch )] = glyphIndex;
			mGlyphToChar[glyphIndex] = static_cast<SdfText::Font::Char>( ch );
		}

		// Get texture atlas - will build if necessary
		mTextureAtlases = SdfTextManager::instance()->getTextureAtlas( face, format, utf8Chars, glyphIndices );

		// Build glyph metrics
		{
			FT_Face face = mFont.getFace();
			for( const auto &glyphIndex : glyphIndices ) {
				FT_Load_Glyph( face, glyphIndex, FT_LOAD_DEFAULT );
				FT_GlyphSlot slot = face->glyph;
				SdfText::Font::GlyphMetrics glyphMetrics;
				glyphMetrics.advance = vec2( slot->linearHoriAdvance, slot->linearVertAdvance ) / 65536.0f;
				glyphMetrics.minimum = vec2( slot->metrics.horiBearingX, slot->metrics.vertBearingY - slot->metrics.height ) / 65536.0f;
				glyphMetrics.maximum = vec2( slot->metrics.horiBearingX + slot->metrics.width, slot->metrics.vertBearingY ) / 65536.0f;
				mGlyphMetrics[glyphIndex] = glyphMetrics;
			}
		}
	}
}

SdfText::~SdfText()
{
}

SdfTextRef SdfText::create( const SdfText::Font &font, const Format &format, const std::string &supportedChars )
{
	SdfTextRef result = SdfTextRef( new SdfText( font, format, supportedChars ) );
	return result;
}

cinder::gl::SdfTextRef SdfText::create( const fs::path& filePath, const SdfText::Font &font, const Format &format, const std::string &utf8Chars )
{
	SdfTextRef result;
	if( fs::exists( filePath ) ) {
		result = SdfText::load( filePath, font.getSize() );
	}
	else {
		result = create( font, format, utf8Chars );
		if( result ) {
			// Save first
			SdfText::save( filePath, result );
			// Load to ensure parity
			result = SdfText::load( filePath, font.getSize() );
		}
	}
	return result;
}

void SdfText::save(const ci::DataTargetRef& target, const SdfTextRef& sdfText)
{
	const uint32_t kCurrentVersion = 0x00000001;

	if( ! target ) {
		throw ci::Exception( "Invalid data target" );
	}

	auto os = target->getStream();
	if( ! os ) {
		throw ci::Exception( "Invalid out stream" );
	}

	if( ! sdfText->mTextureAtlases ) {
		throw ci::Exception( "No texture atlases" );
	}

	// File ident: SDFT
	os->write( static_cast<uint8_t>( 'S' ) );
	os->write( static_cast<uint8_t>( 'D' ) );
	os->write( static_cast<uint8_t>( 'F' ) );
	os->write( static_cast<uint8_t>( 'T' ) );

	// Version
	os->writeLittle( kCurrentVersion );

	// Name
	{
		const std::string name = sdfText->getFont().getName();
		const uint32_t nameLength = static_cast<uint32_t>( name.length() );
		os->writeLittle( nameLength );
		os->writeData( name.data(), nameLength );
	}

	// Size
	os->writeLittle( sdfText->getFont().getSize() );
	// Leading
	os->writeLittle( sdfText->getFont().getLeading() );
	// Height
	os->writeLittle( sdfText->getFont().getHeight() );
	// Ascent
	os->writeLittle( sdfText->getFont().getAscent() );
	// Descent
	os->writeLittle( sdfText->getFont().getDescent() );

	// Char/glyph maps
	{
		// Char/glyph ident: CHGL
		os->write( static_cast<uint8_t>( 'C' ) );
		os->write( static_cast<uint8_t>( 'H' ) );
		os->write( static_cast<uint8_t>( 'G' ) );
		os->write( static_cast<uint8_t>( 'L' ) );

		// Number of chars
		const uint32_t numChars = static_cast<uint32_t>( sdfText->mCharToGlyph.size() );
		os->writeLittle( numChars );
		// Chars/glyphs
		for( const auto& it : sdfText->mCharToGlyph ) {
			uint32_t ch = static_cast<uint32_t>( it.first );
			const SdfText::Font::Glyph& glyph = it.second;
			os->writeLittle( ch );
			os->writeLittle( glyph );
		}		
	}

	// Glyph metrics
	{
		// Glyph metrics ident: GLMT
		os->write( static_cast<uint8_t>( 'G' ) );
		os->write( static_cast<uint8_t>( 'L' ) );
		os->write( static_cast<uint8_t>( 'M' ) );
		os->write( static_cast<uint8_t>( 'T' ) );
	
		// Number of glyph metrics
		const uint32_t numGlyphMetrics = static_cast<uint32_t>( sdfText->mGlyphMetrics.size() );
		os->writeLittle( numGlyphMetrics );
		// Glyph metrics
		for( const auto& it : sdfText->mGlyphMetrics ) {
			const SdfText::Font::Glyph& glyph = it.first;
			const SdfText::Font::GlyphMetrics& metrics = it.second;
			os->writeLittle( glyph );
			os->writeLittle( metrics.advance.x );
			os->writeLittle( metrics.advance.y );
			os->writeLittle( metrics.minimum.x );
			os->writeLittle( metrics.minimum.y );
			os->writeLittle( metrics.maximum.x );
			os->writeLittle( metrics.maximum.y );
		}
	}

	// Texture atlases
	{
		// Texture atlases ident: TXAT
		os->write( static_cast<uint8_t>( 'T' ) );
		os->write( static_cast<uint8_t>( 'X' ) );
		os->write( static_cast<uint8_t>( 'A' ) );
		os->write( static_cast<uint8_t>( 'T' ) );

		// SDF scale
		os->writeLittle( sdfText->mTextureAtlases->mSdfScale.x );
		os->writeLittle( sdfText->mTextureAtlases->mSdfScale.y );
		// SDF padding
		os->writeLittle( sdfText->mTextureAtlases->mSdfPadding.x );
		os->writeLittle( sdfText->mTextureAtlases->mSdfPadding.y );
		// SDF bitmap size
		os->writeLittle( sdfText->mTextureAtlases->mSdfBitmapSize.x );
		os->writeLittle( sdfText->mTextureAtlases->mSdfBitmapSize.y );
		// Max glyph size
		os->writeLittle( sdfText->mTextureAtlases->mMaxGlyphSize.x );
		os->writeLittle( sdfText->mTextureAtlases->mMaxGlyphSize.y );
		// Max ascent
		os->writeLittle( sdfText->mTextureAtlases->mMaxAscent );
		// Max descent
		os->writeLittle( sdfText->mTextureAtlases->mMaxDescent );
	
		// Number of glyphs
		const uint32_t numChars = static_cast<uint32_t>( sdfText->mTextureAtlases->mGlyphInfo.size() );
		os->writeLittle( numChars );
		// Glyph info
		for( const auto& it : sdfText->mTextureAtlases->mGlyphInfo ) {
			const SdfText::Font::Glyph& glyph = it.first;
			const SdfText::Font::GlyphInfo& glyphInfo = it.second;
			os->writeLittle( glyph );
			os->writeLittle( glyphInfo.mTextureIndex );
			os->writeLittle( glyphInfo.mTexCoords.x1 );
			os->writeLittle( glyphInfo.mTexCoords.y1 );
			os->writeLittle( glyphInfo.mTexCoords.x2 );
			os->writeLittle( glyphInfo.mTexCoords.y2 );
			os->writeLittle( glyphInfo.mOriginOffset.x );
			os->writeLittle( glyphInfo.mOriginOffset.y );
			os->writeLittle( glyphInfo.mSize.x );
			os->writeLittle( glyphInfo.mSize.y );
		}

		// Number of textures
		const uint32_t numTextures = static_cast<uint32_t>( sdfText->mTextureAtlases->mTextures.size() );
		os->writeLittle( numTextures );
		// Textures
		for( const auto& tex : sdfText->mTextureAtlases->mTextures ) {
			// Write texture to PNG using memory buffer
			ImageSourceRef pngSource = tex->createSource();
			OStreamMemRef pngStream = OStreamMem::create();
			DataTargetStreamRef pngTarget = DataTargetStream::createRef( pngStream );
			writeImage( pngTarget, pngSource, ImageTarget::Options(), "png" );					
			// PNG ident: PNG
			os->write( static_cast<uint8_t>( 'P' ) );
			os->write( static_cast<uint8_t>( 'N' ) );
			os->write( static_cast<uint8_t>( 'G' ) );
			os->write( static_cast<uint8_t>( 'F' ) );			
			// Create buffer
			const size_t pngBufferSize = static_cast<size_t>( pngStream->tell() );
			BufferRef buffer = ci::Buffer::create( pngStream->getBuffer(), pngBufferSize );
			// Write buffer
			os->writeLittle( static_cast<uint32_t>( buffer->getSize() ) );
			os->write( *buffer );
		}
	}
}

void SdfText::save( const ci::fs::path& filePath, const SdfTextRef& sdfText )
{
	SdfText::save( ci::writeFile( filePath, true ), sdfText );
}

SdfTextRef SdfText::load( const ci::DataSourceRef& source, float size )
{
	ci::IStreamRef is = source->createStream();
	if( ! is ) {
		throw ci::Exception( "Invalid source" );
	}
	
	// File ident: SDFT
	{
		uint8_t ident[4];
		is->readData( ident, 4 );
		if( std::string( "SDFT") != std::string( reinterpret_cast<const char*>( ident ), 4 ) ) {
			throw ci::Exception( "Not a SDF text cache file" );
		}
	}

	// Version
	uint32_t version = 0;
	is->readLittle( &version );

	// Font
	SdfText::Font font;

	// Name
	{
		uint32_t nameLength = 0;
		is->readLittle( &nameLength );
		std::vector<uint8_t> nameBuf( nameLength );
		is->readData( nameBuf.data(), nameLength );
		font.mName = std::string( reinterpret_cast<const char *>( nameBuf.data() ), nameBuf.size() );
	}

	// Size
	is->readLittle( &(font.mSize) );
	// Leading
	is->readLittle( &(font.mLeading) );
	// Height
	is->readLittle( &(font.mHeight) );
	// Ascent
	is->readLittle( &(font.mAscent) );
	// Descent
	is->readLittle( &(font.mDescent) );

	// Override font size if it's requested
	float fontSizeScale = 1.0f;
	if( size > 0.0f ) {
		fontSizeScale = size / font.mSize;
		font.mSize = size;
	}

	// Create SdfText
	SdfText::Format format = SdfText::Format();
	SdfTextRef sdfText = SdfTextRef( new SdfText( font, format, "", false ) );

	// Char/glyph maps
	{
		// Char/glyph ident: CHGL
		uint8_t ident[4];
		is->readData( ident, 4 );
		if( std::string( "CHGL") != std::string( reinterpret_cast<const char*>( ident ), 4 ) ) {
			throw ci::Exception( "Char/glyph ident not found" );
		}

		// Number of chars
		uint32_t numChars = 0;
		is->readLittle( &numChars );
		// Chars/glyphs
		for( uint32_t i = 0; i < numChars; ++i ) {
			uint32_t ch = 0;
			SdfText::Font::Glyph glyph = 0;
			is->readLittle( &ch );
			is->readLittle( &glyph );
			SdfText::Font::Char sdftCh = static_cast<SdfText::Font::Char>( ch );
			sdfText->mCharToGlyph[sdftCh] = glyph;
			sdfText->mGlyphToChar[glyph] = sdftCh;
		}		
	}

	// Glyph metrics
	{
		// File ident: SDFT
		uint8_t ident[4];
		is->readData( ident, 4 );
		if( std::string( "GLMT") != std::string( reinterpret_cast<const char*>( ident ), 4 ) ) {
			throw ci::Exception( "Glyph metrics ident not found" );
		}

		// Number of glyph metrics
		uint32_t numGlyphMetrics = 0;
		is->readLittle( &numGlyphMetrics );
		// Glyph metrics
		for( uint32_t i = 0; i < numGlyphMetrics; ++i ) {
			SdfText::Font::Glyph glyph = 0;
			SdfText::Font::GlyphMetrics metrics = {};
			is->readLittle( &glyph );
			is->readLittle( &(metrics.advance.x) );
			is->readLittle( &(metrics.advance.y) );
			is->readLittle( &( metrics.minimum.x ) );
			is->readLittle( &( metrics.minimum.y ) );
			is->readLittle( &( metrics.maximum.x ) );
			is->readLittle( &( metrics.maximum.y ) );
			metrics.advance *= fontSizeScale;
			metrics.minimum *= fontSizeScale;
			metrics.maximum *= fontSizeScale;
			sdfText->mGlyphMetrics[glyph] = metrics;
		}
	}

	// Texture atlases
	{
		// File ident: TXAT
		uint8_t ident[4];
		is->readData( ident, 4 );
		if( std::string( "TXAT") != std::string( reinterpret_cast<const char*>( ident ), 4 ) ) {
			throw ci::Exception( "Texture atlas ident not found" );
		}

		// Create TextureAtlas
		TextureAtlasRef textureAtlases = TextureAtlasRef( new TextureAtlas() );

		// SDF scale
		is->readLittle( &(textureAtlases->mSdfScale.x) );
		is->readLittle( &(textureAtlases->mSdfScale.y) );
		// SDF padding
		is->readLittle( &(textureAtlases->mSdfPadding.x) );
		is->readLittle( &(textureAtlases->mSdfPadding.y) );
		// SDF bitmap size
		is->readLittle( &(textureAtlases->mSdfBitmapSize.x) );
		is->readLittle( &(textureAtlases->mSdfBitmapSize.y) );
		// Max glyph size
		is->readLittle( &(textureAtlases->mMaxGlyphSize.x) );
		is->readLittle( &(textureAtlases->mMaxGlyphSize.y) );
		// Max ascent
		is->readLittle( &(textureAtlases->mMaxAscent) );
		// Max descent
		is->readLittle( &(textureAtlases->mMaxDescent) );

		// Number of glyphs
		uint32_t numGlyphs = 0;
		is->readLittle( &numGlyphs );
		// Glyph info
		for( uint32_t i = 0; i < numGlyphs; ++i ) {
			SdfText::Font::Glyph glyph = 0;
			SdfText::Font::GlyphInfo glyphInfo = {};
			is->readLittle( &glyph );
			is->readLittle( &(glyphInfo.mTextureIndex) );
			is->readLittle( &(glyphInfo.mTexCoords.x1) );
			is->readLittle( &(glyphInfo.mTexCoords.y1) );
			is->readLittle( &(glyphInfo.mTexCoords.x2) );
			is->readLittle( &(glyphInfo.mTexCoords.y2) );
			is->readLittle( &(glyphInfo.mOriginOffset.x) );
			is->readLittle( &(glyphInfo.mOriginOffset.y) );
			is->readLittle( &(glyphInfo.mSize.x) );
			is->readLittle( &(glyphInfo.mSize.y) );
			textureAtlases->mGlyphInfo[glyph] = glyphInfo;
		}

		// Number of textures
		uint32_t numTextures = 0;
		is->readLittle( &numTextures );
		// Textures
		for( uint32_t i = 0; i < numTextures; ++i ) {		
			// PNG ident: PNGF
			uint8_t ident[4];
			is->readData( ident, 4 );
			if( std::string( "PNGF") != std::string( reinterpret_cast<const char*>( ident ), 4 ) ) {
				throw ci::Exception( "PNG ident not found" );
			}
			// Read buffer
			uint32_t bufferSize = 0;
			is->readLittle( &bufferSize );
			BufferRef buffer = Buffer::create( bufferSize );
			is->readData( buffer->getData(), buffer->getSize() );
			// Create surface
			ImageSourceRef pngSource = loadImage( DataSourceBuffer::create( buffer ) );
			gl::TextureRef tex = gl::Texture2d::create( pngSource );
			// Add texture
			textureAtlases->mTextures.push_back( tex );
		}

		sdfText->mTextureAtlases = textureAtlases;
	}

	return sdfText;
}

SdfTextRef SdfText::load( const ci::fs::path& filePath, float size )
{
	return SdfText::load( ci::DataSourcePath::create( filePath ), size );
}

void SdfText::drawGlyphs( const SdfText::Font::GlyphMeasuresList &glyphMeasures, const vec2 &baselineIn, const DrawOptions &options, const std::vector<ColorA8u> &colors )
{
	const auto& textures = mTextureAtlases->mTextures;
	const auto& glyphMap = mTextureAtlases->mGlyphInfo;
	const auto& sdfScale = mTextureAtlases->mSdfScale;
	const auto& sdfPadding = mTextureAtlases->mSdfPadding;
	const auto& sdfBitmapSize = mTextureAtlases->mSdfBitmapSize;

	if( textures.empty() ) {
		return;
	}

	if( ! colors.empty() ) {
		assert( glyphMeasures.size() == colors.size() );
	}

	auto shader = options.getGlslProg();
	if( ! shader ) {
		shader = SdfText::defaultShader();
	}
	ScopedTextureBind texBindScp( textures[0] );
	ScopedGlslProg glslScp( shader );

	vec2 baseline = baselineIn;

	if( ! options.getGlslProg() ) {
		shader->uniform( "uFgColor", gl::context()->getCurrentColor() );
		shader->uniform( "uPremultiply", options.getPremultiply() ? 1.0f : 0.0f );
		shader->uniform( "uGamma", options.getGamma() );
#if defined(CINDER_GL_ES)
		shader->uniform( "uTexSize", vec2( textures[0]->getSize() ) );
#endif
	}

	const vec2 fontRenderScale = vec2( mFont.getSize() ) / ( 32.0f * mTextureAtlases->mSdfScale );
	const vec2 fontOriginScale = vec2( mFont.getSize() ) / 32.0f;

	const float scale = options.getScale();
	for( size_t texIdx = 0; texIdx < textures.size(); ++texIdx ) {
		std::vector<float> verts, texCoords;
		std::vector<ColorA8u> vertColors;
		const gl::TextureRef &curTex = textures[texIdx];

		std::vector<uint32_t> indices;
		uint32_t curIdx = 0;
		GLenum indexType = GL_UNSIGNED_INT;

		if( options.getPixelSnap() ) {
			baseline = vec2( floor( baseline.x ), floor( baseline.y ) );
		}
			
		for( std::vector<std::pair<SdfText::Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
			SdfText::Font::GlyphInfoMap::const_iterator glyphInfoIt = glyphMap.find( glyphIt->first );
			if(  glyphInfoIt == glyphMap.end() ) {
				continue;
			}
				
			const auto &glyphInfo = glyphInfoIt->second;
			if( glyphInfo.mTextureIndex != texIdx ) {
				continue;
			}

			const auto &originOffset = glyphInfo.mOriginOffset;

			Rectf srcTexCoords = curTex->getAreaTexCoords( glyphInfo.mTexCoords );
			Rectf destRect = Rectf( glyphInfo.mTexCoords );
			destRect.scale( scale );
			destRect -= destRect.getUpperLeft();
			vec2 offset = vec2( 0, -( destRect.getHeight() ) );
			// Reverse the transformation applied during SDF generation
			float tx = sdfPadding.x;
			float ty = std::fabs( originOffset.y ) + sdfPadding.y;
			offset += scale * sdfScale * vec2( -tx, ty );
			// Use origin scale for horizontal offset
			offset += scale * fontOriginScale * vec2( originOffset.x, 0.0f );
			destRect += offset;
			destRect.scale( fontRenderScale );

			destRect += glyphIt->second * scale;
			destRect += baseline;
			
			verts.push_back( destRect.getX2() ); verts.push_back( destRect.getY1() );
			verts.push_back( destRect.getX1() ); verts.push_back( destRect.getY1() );
			verts.push_back( destRect.getX2() ); verts.push_back( destRect.getY2() );
			verts.push_back( destRect.getX1() ); verts.push_back( destRect.getY2() );

			texCoords.push_back( srcTexCoords.getX2() ); texCoords.push_back( srcTexCoords.getY1() );
			texCoords.push_back( srcTexCoords.getX1() ); texCoords.push_back( srcTexCoords.getY1() );
			texCoords.push_back( srcTexCoords.getX2() ); texCoords.push_back( srcTexCoords.getY2() );
			texCoords.push_back( srcTexCoords.getX1() ); texCoords.push_back( srcTexCoords.getY2() );
			
			if( ! colors.empty() ) {
				for( int i = 0; i < 4; ++i ) {
					vertColors.push_back( colors[glyphIt-glyphMeasures.begin()] );
				}
			}

			indices.push_back( curIdx + 0 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 2 );
			indices.push_back( curIdx + 2 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 3 );
			curIdx += 4;
		}
		
		if( curIdx == 0 ) {
			continue;
		}
		
		curTex->bind();
		auto ctx = gl::context();
		size_t dataSize = (verts.size() + texCoords.size()) * sizeof(float) + vertColors.size() * sizeof(ColorA8u);
		gl::ScopedVao vaoScp( ctx->getDefaultVao() );
		ctx->getDefaultVao()->replacementBindBegin();
		VboRef defaultElementVbo = ctx->getDefaultElementVbo( indices.size() * sizeof(curIdx) );
		VboRef defaultArrayVbo = ctx->getDefaultArrayVbo( dataSize );

		ScopedBuffer vboArrayScp( defaultArrayVbo );
		ScopedBuffer vboElScp( defaultElementVbo );

		size_t dataOffset = 0;
		int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
		if( posLoc >= 0 ) {
			enableVertexAttribArray( posLoc );
			vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
			defaultArrayVbo->bufferSubData( dataOffset, verts.size() * sizeof(float), verts.data() );
			dataOffset += verts.size() * sizeof(float);
		}
		int texLoc = shader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
		if( texLoc >= 0 ) {
			enableVertexAttribArray( texLoc );
			vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)dataOffset );
			defaultArrayVbo->bufferSubData( dataOffset, texCoords.size() * sizeof(float), texCoords.data() );
			dataOffset += texCoords.size() * sizeof(float);
		}
		if( ! vertColors.empty() ) {
			int colorLoc = shader->getAttribSemanticLocation( geom::Attrib::COLOR );
			if( colorLoc >= 0 ) {
				enableVertexAttribArray( colorLoc );
				vertexAttribPointer( colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)dataOffset );
				defaultArrayVbo->bufferSubData( dataOffset, vertColors.size() * sizeof(ColorA8u), vertColors.data() );
				dataOffset += vertColors.size() * sizeof(ColorA8u);				
			}
		}

		defaultElementVbo->bufferSubData( 0, indices.size() * sizeof(curIdx), indices.data() );
		ctx->getDefaultVao()->replacementBindEnd();
		gl::setDefaultShaderVars();
		ctx->drawElements( GL_TRIANGLES, (GLsizei)indices.size(), indexType, 0 );
	}
}

void SdfText::drawGlyphs( const SdfText::Font::GlyphMeasuresList &glyphMeasures, const Rectf &clip, vec2 offset, const DrawOptions &options, const std::vector<ColorA8u> &colors )
{
	const auto& textures = mTextureAtlases->mTextures;
	const auto& glyphMap = mTextureAtlases->mGlyphInfo;
	const auto& sdfPadding = mTextureAtlases->mSdfPadding;
	const auto& sdfBitmapSize = mTextureAtlases->mSdfBitmapSize;

	if( textures.empty() ) {
		return;
	}

	if( ! colors.empty() ) {
		assert( glyphMeasures.size() == colors.size() );
	}

	auto shader = options.getGlslProg();
	if( ! shader ) {
		shader = SdfText::defaultShader();
	}
	ScopedTextureBind texBindScp( textures[0] );
	ScopedGlslProg glslScp( shader );

	if( ! options.getGlslProg() ) {
		shader->uniform( "uFgColor", gl::context()->getCurrentColor() );
		shader->uniform( "uPremultiply", options.getPremultiply() ? 1.0f : 0.0f );
		shader->uniform( "uGamma", options.getGamma() );
#if defined(CINDER_GL_ES)
		shader->uniform( "uTexSize", vec2( textures[0]->getSize() ) );
#endif
	}

	const vec2 fontRenderScale = vec2( mFont.getSize() ) / ( 32.0f * mTextureAtlases->mSdfScale );
	const vec2 fontOriginScale = vec2( mFont.getSize() ) / 32.0f;

	const float scale = options.getScale();
	for( size_t texIdx = 0; texIdx < textures.size(); ++texIdx ) {
		std::vector<float> verts, texCoords;
		std::vector<ColorA8u> vertColors;
		const gl::TextureRef &curTex = textures[texIdx];

		std::vector<uint32_t> indices;
		uint32_t curIdx = 0;
		GLenum indexType = GL_UNSIGNED_INT;

		if( options.getPixelSnap() ) {
			offset = vec2( floor( offset.x ), floor( offset.y ) );
		}

		for( std::vector<std::pair<Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
			SdfText::Font::GlyphInfoMap::const_iterator glyphInfoIt = glyphMap.find( glyphIt->first );
			if( glyphInfoIt == glyphMap.end() ) {
				continue;
			}
				
			const auto &glyphInfo = glyphInfoIt->second;
			if( glyphInfo.mTextureIndex != texIdx ) {
				continue;
			}

			Rectf srcTexCoords = curTex->getAreaTexCoords( glyphInfo.mTexCoords );
			Rectf destRect( glyphInfo.mTexCoords );
			destRect.scale( fontRenderScale );
			destRect -= destRect.getUpperLeft();
			destRect.scale( scale );
			destRect += glyphIt->second * scale;
			destRect += vec2( offset.x, offset.y );
			vec2 originOffset = fontOriginScale * glyphInfo.mOriginOffset;
			destRect += vec2( floor( originOffset.x + 0.5f ), floor( -originOffset.y ) ) * scale;
			destRect += fontRenderScale * vec2( -sdfPadding.x, -sdfPadding.y );
			if( options.getPixelSnap() ) {
				destRect -= vec2( destRect.x1 - floor( destRect.x1 ), destRect.y1 - floor( destRect.y1 ) );	
			}

			// clip
			Rectf clipped( destRect );
			if( options.getClipHorizontal() ) {
				clipped.x1 = std::max( destRect.x1, clip.x1 );
				clipped.x2 = std::min( destRect.x2, clip.x2 );
			}
			if( options.getClipVertical() ) {
				clipped.y1 = std::max( destRect.y1, clip.y1 );
				clipped.y2 = std::min( destRect.y2, clip.y2 );
			}
			
			if( clipped.x1 >= clipped.x2 || clipped.y1 >= clipped.y2 ) {
				continue;
			}

			verts.push_back( clipped.getX2() ); verts.push_back( clipped.getY1() );
			verts.push_back( clipped.getX1() ); verts.push_back( clipped.getY1() );
			verts.push_back( clipped.getX2() ); verts.push_back( clipped.getY2() );
			verts.push_back( clipped.getX1() ); verts.push_back( clipped.getY2() );

			vec2 coordScale = vec2( srcTexCoords.getWidth() / destRect.getWidth(), srcTexCoords.getHeight() / destRect.getHeight() );
			srcTexCoords.x1 = srcTexCoords.x1 + ( clipped.x1 - destRect.x1 ) * coordScale.x;
			srcTexCoords.x2 = srcTexCoords.x1 + ( clipped.x2 - clipped.x1  ) * coordScale.x;
			srcTexCoords.y1 = srcTexCoords.y1 + ( clipped.y1 - destRect.y1 ) * coordScale.y;
			srcTexCoords.y2 = srcTexCoords.y1 + ( clipped.y2 - clipped.y1  ) * coordScale.y;

			texCoords.push_back( srcTexCoords.getX2() ); texCoords.push_back( srcTexCoords.getY1() );
			texCoords.push_back( srcTexCoords.getX1() ); texCoords.push_back( srcTexCoords.getY1() );
			texCoords.push_back( srcTexCoords.getX2() ); texCoords.push_back( srcTexCoords.getY2() );
			texCoords.push_back( srcTexCoords.getX1() ); texCoords.push_back( srcTexCoords.getY2() );

			if( ! colors.empty() ) {
				for( int i = 0; i < 4; ++i ) {
					vertColors.push_back( colors[glyphIt-glyphMeasures.begin()] );
				}
			}
			
			indices.push_back( curIdx + 0 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 2 );
			indices.push_back( curIdx + 2 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 3 );
			curIdx += 4;
		}
		
		if( curIdx == 0 ) {
			continue;
		}
		
		curTex->bind();
		auto ctx = gl::context();
		size_t dataSize = (verts.size() + texCoords.size()) * sizeof(float) + vertColors.size() * sizeof(ColorA8u);
		gl::ScopedVao vaoScp( ctx->getDefaultVao() );
		ctx->getDefaultVao()->replacementBindBegin();
		VboRef defaultElementVbo = ctx->getDefaultElementVbo( indices.size() * sizeof(curIdx) );
		VboRef defaultArrayVbo = ctx->getDefaultArrayVbo( dataSize );

		ScopedBuffer vboArrayScp( defaultArrayVbo );
		ScopedBuffer vboElScp( defaultElementVbo );

		size_t dataOffset = 0;
		int posLoc = shader->getAttribSemanticLocation( geom::Attrib::POSITION );
		if( posLoc >= 0 ) {
			enableVertexAttribArray( posLoc );
			vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
			defaultArrayVbo->bufferSubData( dataOffset, verts.size() * sizeof(float), verts.data() );
			dataOffset += verts.size() * sizeof(float);
		}
		int texLoc = shader->getAttribSemanticLocation( geom::Attrib::TEX_COORD_0 );
		if( texLoc >= 0 ) {
			enableVertexAttribArray( texLoc );
			vertexAttribPointer( texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)dataOffset );
			defaultArrayVbo->bufferSubData( dataOffset, texCoords.size() * sizeof(float), texCoords.data() );
			dataOffset += texCoords.size() * sizeof(float);
		}
		if( ! vertColors.empty() ) {
			int colorLoc = shader->getAttribSemanticLocation( geom::Attrib::COLOR );
			if( colorLoc >= 0 ) {
				enableVertexAttribArray( colorLoc );
				vertexAttribPointer( colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)dataOffset );
				defaultArrayVbo->bufferSubData( dataOffset, vertColors.size() * sizeof(ColorA8u), vertColors.data() );
				dataOffset += vertColors.size() * sizeof(ColorA8u);				
			}
		}

		defaultElementVbo->bufferSubData( 0, indices.size() * sizeof(curIdx), indices.data() );
		ctx->getDefaultVao()->replacementBindEnd();
		gl::setDefaultShaderVars();
		ctx->drawElements( GL_TRIANGLES, (GLsizei)indices.size(), indexType, 0 );
	}
}

void SdfText::drawString( const std::string &str, const vec2 &baseline, const DrawOptions &options )
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, SdfTextBox::GROW ).ligate( options.getLigate() );
	SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );
	drawGlyphs( glyphMeasures, baseline, options );
}

void SdfText::drawString( const std::string &str, const Rectf &fitRect, const vec2 &offset, const DrawOptions &options )
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, (int)fitRect.getHeight() ).ligate( options.getLigate() );
	SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );
	drawGlyphs( glyphMeasures, fitRect, fitRect.getUpperLeft() + offset, options );	
}

void SdfText::drawStringWrapped( const std::string &str, const Rectf &fitRect, const vec2 &offset, const DrawOptions &options )
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( (int)fitRect.getWidth(), (int)fitRect.getHeight() ).ligate( options.getLigate() );
	SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );
	drawGlyphs( glyphMeasures, fitRect.getUpperLeft() + offset, options );
}

std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> SdfText::placeChars( const SdfText::Font::GlyphMeasuresList &glyphMeasures, const vec2 &baselineIn, const DrawOptions &options )
{
	std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> result;

	const auto& textures = mTextureAtlases->mTextures;
	const auto& glyphMap = mTextureAtlases->mGlyphInfo;
	const auto& sdfScale = mTextureAtlases->mSdfScale;
	const auto& sdfPadding = mTextureAtlases->mSdfPadding;
	const auto& sdfBitmapSize = mTextureAtlases->mSdfBitmapSize;

	vec2 baseline = baselineIn;

	const vec2 fontRenderScale = vec2( mFont.getSize() ) / ( 32.0f * mTextureAtlases->mSdfScale );
	const vec2 fontOriginScale = vec2( mFont.getSize() ) / 32.0f;

	const float scale = options.getScale();
	for( size_t texIdx = 0; texIdx < textures.size(); ++texIdx ) {
		std::vector<float> verts, texCoords;
		std::vector<ColorA8u> vertColors;
		const gl::TextureRef &curTex = textures[texIdx];

		if( options.getPixelSnap() ) {
			baseline = vec2( floor( baseline.x ), floor( baseline.y ) );
		}

		std::vector<SdfText::CharPlacement> charPlacements;	
		for( std::vector<std::pair<SdfText::Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
			SdfText::Font::GlyphInfoMap::const_iterator glyphInfoIt = glyphMap.find( glyphIt->first );
			if(  glyphInfoIt == glyphMap.end() ) {
				continue;
			}
				
			const auto &glyphInfo = glyphInfoIt->second;
			if( glyphInfo.mTextureIndex != texIdx ) {
				continue;
			}

			const auto &originOffset = glyphInfo.mOriginOffset;

			Rectf srcTexCoords = curTex->getAreaTexCoords( glyphInfo.mTexCoords );
			Rectf destRect = Rectf( glyphInfo.mTexCoords );
			destRect.scale( scale );
			destRect -= destRect.getUpperLeft();
			vec2 offset = vec2( 0, -( destRect.getHeight() ) );
			// Reverse the transformation applied during SDF generation
			float tx = sdfPadding.x;
			float ty = std::fabs( originOffset.y ) + sdfPadding.y;
			offset += scale * sdfScale * vec2( -tx, ty );
			// Use origin scale for horizontal offset
			offset += scale * fontOriginScale * vec2( originOffset.x, 0.0f );
			destRect += offset;
			destRect.scale( fontRenderScale );

			destRect += glyphIt->second * scale;
			destRect += baseline;
			
			SdfText::CharPlacement place = {};
			place.mGlyph = glyphIt->first;
			place.mSrcTexCoords = srcTexCoords;
			place.mDstRect = destRect;
			charPlacements.push_back( place );
		}

		if( ! charPlacements.empty() ) {
			result.push_back( std::make_pair( static_cast<uint8_t>( texIdx ), charPlacements ) );
		}
	}

	return result;
}

std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> SdfText::placeString( const std::string &str, const vec2 &baseline, const DrawOptions &options )
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, SdfTextBox::GROW ).ligate( options.getLigate() );
	SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );
	std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> result = placeChars( glyphMeasures, baseline, options );
	return result;
}

std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> SdfText::placeStringWrapped( const std::string &str, const Rectf &fitRect, const vec2 &offset, const DrawOptions &options )
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( (int)fitRect.getWidth(), (int)fitRect.getHeight() ).ligate( options.getLigate() );
	SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );
	std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> result = placeChars( glyphMeasures, fitRect.getUpperLeft() + offset, options );
	return result;
}

Rectf SdfText::measureStringImpl( const std::string &str, bool wrapped, const Rectf &fitRect, const DrawOptions &options ) const
{
	SdfTextBox tbox = wrapped ? SdfTextBox( this ).text( str ).size( (int)fitRect.getWidth(), (int)fitRect.getHeight() ).ligate( options.getLigate() )
                              : SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, SdfTextBox::GROW ).ligate( options.getLigate() );
	
    SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );

	const SdfText::Font::GlyphInfoMap& glyphMap = mTextureAtlases->mGlyphInfo;
	const auto& sdfScale = mTextureAtlases->mSdfScale;
	const auto& sdfPadding = mTextureAtlases->mSdfPadding;
	const vec2 fontRenderScale = vec2( mFont.getSize() ) / ( 32.0f * mTextureAtlases->mSdfScale );
	const vec2 fontOriginScale = vec2( mFont.getSize() ) / 32.0f;
	const float scale = options.getScale();

	Rectf result = Rectf( 0, 0, 0, 0 );
    for( std::vector<std::pair<SdfText::Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
        SdfText::Font::GlyphInfoMap::const_iterator glyphInfoIt = glyphMap.find( glyphIt->first );
        if(  glyphInfoIt == glyphMap.end() ) {
            continue;
        }

		const auto &glyphInfo = glyphInfoIt->second;          
        const auto &originOffset = glyphInfo.mOriginOffset;
		const auto &size = glyphInfo.mSize;

        Rectf destRect = Rectf( glyphInfo.mTexCoords );
        destRect.scale( scale );
        destRect -= destRect.getUpperLeft();
        vec2 offset = vec2( 0, -( destRect.getHeight() ) );
        // Reverse the transformation applied during SDF generation
        float tx = sdfPadding.x;
        float ty = std::fabs( originOffset.y ) + sdfPadding.y;
        offset += scale * sdfScale * vec2( -tx, ty );
        // Use origin scale for horizontal offset
        offset += scale * fontOriginScale * vec2( originOffset.x, 0.0f );
        destRect += offset;
        destRect.scale( fontRenderScale );

		destRect += glyphIt->second * scale;

		destRect.x1 += ( ( sdfPadding.x + originOffset.x ) * fontOriginScale.x );
		destRect.y2 -= ( sdfPadding.y * fontOriginScale.y );
		destRect.x2 = destRect.x1 + ( ( size.x + 1.0f ) * fontOriginScale.x );
		destRect.y1 = destRect.y2 - ( ( size.y + 1.0f ) * fontOriginScale.y );
		destRect.x1 -= ( 0.5f * fontOriginScale.x );
		destRect.y1 -= ( 0.5f * fontOriginScale.y );
		destRect.x2 += ( 0.5f * fontOriginScale.x );
		destRect.y2 += ( 0.5f * fontOriginScale.y );

		if( ( result.getWidth() > 0 ) || ( result.getHeight() > 0 ) ) {
			result.x1 = std::min( result.x1, destRect.x1 );
			result.y1 = std::min( result.y1, destRect.y1 );
			result.x2 = std::max( result.x2, destRect.x2 );
			result.y2 = std::max( result.y2, destRect.y2 );
		}
		else {
			result = destRect;
		}
	}

	return result;
}

Rectf SdfText::measureStringBounds( const std::string &str, const DrawOptions &options ) const
{
    Rectf result = measureStringImpl( str, false, Rectf( 0, 0, 0, 0 ), options );
    return result;

/*
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, SdfTextBox::GROW ).ligate( options.getLigate() );
	SdfText::Font::GlyphMeasuresList glyphMeasures = tbox.measureGlyphs( options );

	const SdfText::Font::GlyphInfoMap& glyphMap = mTextureAtlases->mGlyphInfo;
	const auto& sdfScale = mTextureAtlases->mSdfScale;
	const auto& sdfPadding = mTextureAtlases->mSdfPadding;
	const vec2 fontRenderScale = vec2( mFont.getSize() ) / ( 32.0f * mTextureAtlases->mSdfScale );
	const vec2 fontOriginScale = vec2( mFont.getSize() ) / 32.0f;
	const float scale = options.getScale();

	Rectf result = Rectf( 0, 0, 0, 0 );
    for( std::vector<std::pair<SdfText::Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
        SdfText::Font::GlyphInfoMap::const_iterator glyphInfoIt = glyphMap.find( glyphIt->first );
        if(  glyphInfoIt == glyphMap.end() ) {
            continue;
        }

		const auto &glyphInfo = glyphInfoIt->second;          
        const auto &originOffset = glyphInfo.mOriginOffset;
		const auto &size = glyphInfo.mSize;

        Rectf destRect = Rectf( glyphInfo.mTexCoords );
        destRect.scale( scale );
        destRect -= destRect.getUpperLeft();
        vec2 offset = vec2( 0, -( destRect.getHeight() ) );
        // Reverse the transformation applied during SDF generation
        float tx = sdfPadding.x;
        float ty = std::fabs( originOffset.y ) + sdfPadding.y;
        offset += scale * sdfScale * vec2( -tx, ty );
        // Use origin scale for horizontal offset
        offset += scale * fontOriginScale * vec2( originOffset.x, 0.0f );
        destRect += offset;
        destRect.scale( fontRenderScale );

		destRect += glyphIt->second * scale;

		destRect.x1 += ( ( sdfPadding.x + originOffset.x ) * fontOriginScale.x );
		destRect.y2 -= ( sdfPadding.y * fontOriginScale.y );
		destRect.x2 = destRect.x1 + ( ( size.x + 1.0f ) * fontOriginScale.x );
		destRect.y1 = destRect.y2 - ( ( size.y + 1.0f ) * fontOriginScale.y );
		destRect.x1 -= ( 0.5f * fontOriginScale.x );
		destRect.y1 -= ( 0.5f * fontOriginScale.y );
		destRect.x2 += ( 0.5f * fontOriginScale.x );
		destRect.y2 += ( 0.5f * fontOriginScale.y );

		if( ( result.getWidth() > 0 ) || ( result.getHeight() > 0 ) ) {
			result.x1 = std::min( result.x1, destRect.x1 );
			result.y1 = std::min( result.y1, destRect.y1 );
			result.x2 = std::max( result.x2, destRect.x2 );
			result.y2 = std::max( result.y2, destRect.y2 );
		}
		else {
			result = destRect;
		}
	}

	return result;
*/
}

Rectf SdfText::measureStringBoundsWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options ) const
{
    Rectf result = measureStringImpl( str, true, fitRect, options );
    return result;
}

vec2 SdfText::measureString( const std::string &str, const DrawOptions &options ) const
{
    Rectf bounds = measureStringImpl( str, false, Rectf( 0, 0, 0, 0 ), options );
	vec2 result = vec2( bounds.getWidth(), bounds.getHeight() );
	return result;
}

vec2 SdfText::measureStringWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options ) const
{
    Rectf bounds = measureStringImpl( str, true, fitRect, options );
	vec2 result = vec2( bounds.getWidth(), bounds.getHeight() );
	return result;
}

std::vector<std::pair<SdfText::Font::Glyph, vec2>> SdfText::getGlyphPlacements( const std::string &str, const DrawOptions &options ) const
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, SdfTextBox::GROW ).ligate( options.getLigate() );
	return tbox.measureGlyphs(  options );
}

std::vector<std::pair<SdfText::Font::Glyph, vec2>> SdfText::getGlyphPlacements( const std::string &str, const Rectf &fitRect, const DrawOptions &options ) const
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( SdfTextBox::GROW, (int)fitRect.getHeight() ).ligate( options.getLigate() );
	return tbox.measureGlyphs( options );
}

std::vector<std::pair<SdfText::Font::Glyph, vec2>> SdfText::getGlyphPlacementsWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options ) const
{
	SdfTextBox tbox = SdfTextBox( this ).text( str ).size( (int)fitRect.getWidth(), (int)fitRect.getHeight() ).ligate( options.getLigate() );
	return tbox.measureGlyphs( options );
}

std::string SdfText::defaultChars()
{ 
	return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890().?!,:;'\"&*=+-/\\|@#_[]<>%^llflfiphrids\303\251\303\241\303\250\303\240"; 
}

uint32_t SdfText::getNumTextures() const
{
	return static_cast<uint32_t>( mTextureAtlases->mTextures.size() );
}

const gl::TextureRef& SdfText::getTexture(uint32_t n) const
{
	return mTextureAtlases->mTextures[static_cast<size_t>( n )];
}

gl::GlslProgRef SdfText::defaultShader()
{
	if( ! sDefaultShader ) {
		try {
			sDefaultShader = gl::GlslProg::create( kSdfVertShader, kSdfFragShader );
		}
		catch( const std::exception& e ) {
			CI_LOG_E( "SdfText::defaultShader error: " << e.what() );
		}
	}
	return sDefaultShader;
}

}} // namespace cinder::gl

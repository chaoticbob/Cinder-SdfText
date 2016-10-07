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

#pragma once

#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

#include <unordered_map>

typedef struct FT_FaceRec_*  FT_Face;

namespace cinder { namespace gl {

class SdfText;
using SdfTextRef = std::shared_ptr<SdfText>;

//! \class SdfText
//!
//!
class SdfText {
public:
	typedef enum Alignment { LEFT, CENTER, RIGHT } Alignment;

	//! \class Options
	//!
	//!
	class Format {
	public:
		Format() {}
		virtual ~Format() {}

		//! Sets the width of the textures created internally for glyphs. Default \c 1024
		Format&			textureWidth( int32_t textureWidth ) { mTextureSize.x = textureWidth; return *this; }
		//! Returns the width of the textures created internally for glyphs. Default \c 1024
		int32_t			getTextureWidth() const { return mTextureSize.x; }
		//! Sets the height of the textures created internally for glyphs. Default \c 1024
		Format&			textureHeight( int32_t textureHeight ) { mTextureSize.y = textureHeight; return *this; }
		//! Returns the width of the textures created internally for glyphs. Default \c 1024
		int32_t			getTextureHeight() const { return mTextureSize.y; }
		//! Returns the size of the textures created internally for glyphs. Default \c 1024x1024
		const ivec2&	getTextureSize() const { return mTextureSize; }

		Format&			sdfScale( const vec2 &value ) { mSdfScale = value; return *this; }
		Format&			sdfScale( float value ) { return sdfScale( vec2( value ) ); }
		const vec2&		getSdfScale() const { return mSdfScale; }

		Format&			sdfPadding( const ivec2 &value ) { mSdfScale = value; return *this; }
		const ivec2&	getSdfPadding() const { return mSdfPadding; }

		Format&			sdfRange( float value ) { mSdfRange = value; return *this; }
		float			getSdfRange() const { return mSdfRange; }

		Format&			sdfAngle( float value ) { mSdfAngle = value; return *this; }
		float			getSdfAngle() const { return mSdfAngle; }

		Format&			sdfTileSpacing( const ivec2& value ) { mSdfTileSpacing = value; return *this; }
		const ivec2&	getSdfTileSpacing() const { return mSdfTileSpacing; }

	private:
		ivec2			mTextureSize = ivec2( 1024 );
		vec2			mSdfScale = vec2( 2.0f );
		ivec2			mSdfPadding = vec2( 2.0f );
		float			mSdfRange = 4.0f;
		float			mSdfAngle = 3.0f;
		ivec2			mSdfTileSpacing = ivec2( 1 );
	};

	// ---------------------------------------------------------------------------------------------

	//! \class DrawOptions
	//!
	//!
	struct DrawOptions {
		DrawOptions() : mClipHorizontal( true ), mClipVertical( true ), mPixelSnap( true ), mLigate( false ), mScale( 1 ) {}

		//! Returns whether the output clips horizontally
		bool			getClipHorizontal() const { return mClipHorizontal; }		
		//! Sets whether the output clips horizontally
		DrawOptions&	clipHorizontal( bool clipH = true ) { mClipHorizontal = clipH; return *this; }

		//! Returns whether the output clips vertically
		bool			getClipVertical() const { return mClipVertical; }		
		//! Sets whether the output clips vertically
		DrawOptions&	clipVertical( bool clipV = true ) { mClipVertical = clipV; return *this; }

		//! Returns whether the output glyphs are snapped to pixel boundaries. This sharpens static text but prevents subpixel animation
		bool			getPixelSnap() const { return mPixelSnap; }		
		//! Sets whether the output glyphs are snapped to pixel boundaries. This sharpens static text but prevents subpixel animation
		DrawOptions&	pixelSnap( bool pixelSnap = true ) { mPixelSnap = pixelSnap; return *this; }

		//! Returns whether advanced ligatures are used, which must have been instantiated by the \a supportedChars parameter of the TextureFont::create() call. Default to \c false.
		bool			getLigate() const { return mLigate; }
		//! Sets whether advanced ligatures are used, which must have been instantiated by the \a supportedChars parameter of the TextureFont::create() call. Default to \c false.
		DrawOptions&	ligate( bool useLigatures = true ) { mLigate = useLigatures; return *this; }

		//! Returns the scale at which the type is rendered. 2 is double size. Default \c 1
		float			getScale() const { return mScale; }
		//! Sets the scale at which the type is rendered. 2 is double size. Default \c 1
		DrawOptions&	scale( float sc ) { mScale = sc; return *this; }

		//! Returns the leading (aka line gap) used adjust the line height when wrapping. Default \c 0
		float			getLeading() const { return mLeading; }
		//! Sets the leading (aka line gap) used adjust the line height when wrapping. Default \c 0
		DrawOptions&	leading( float value ) { mLeading = value; return *this; }

		//! Returns the horizontal alignment (LEFT, CENTER, RIGHT) of the type. Default \c LEFT
		Alignment		getAlignment() const { return mAlign; }
		//! Sets the horizontal alignment (LEFT, CENTER, RIGHT) of the type. Default \c LEFT
		DrawOptions&	alignment( Alignment align ) { mAlign = align; return *this; }

		//! Returns whether the type is flushed to both the left and right sides. Default \c false
		bool			getJustify() const { return mJustify; }
		//! Sets whether the type is flushed to both the left and right sides. Default \c false
		DrawOptions&	justify( bool enabled = true ) { mJustify = enabled; return *this; }

		//! Sets whether the TextureFont render premultiplied output. Default \c false
		DrawOptions&	premultiply( bool premult = true ) { mPremultiply = premult; return *this; }
		//! Returns whether the TextureFont renders premultiplied output. Default \c false
		bool			getPremultiply() const { return mPremultiply; }

		//! Returns the gamma value that's used when drawing. \Default 2.2
		float			getGamma() const { return mGamma; }
		//! Sets the gamma value that's used when drawing. \Default 2.2
		DrawOptions&	gamma( float value ) { mGamma = value; return *this; }

		//! Returns the user-specified glsl program if set. Otherwise returns nullptr.
		const GlslProgRef&	getGlslProg() const { return mGlslProg; }
		//! Sets a custom shader to use when the type is rendered.
		DrawOptions&		glslProg( const GlslProgRef &glslProg ) { mGlslProg = glslProg; return *this; }

	  protected:
		bool			mClipHorizontal, mClipVertical, mPixelSnap, mLigate;
		float			mScale = 2.0;
		float			mLeading = 0.0f;
		bool			mPremultiply = false;
		bool			mJustify = false;
		float			mGamma = 2.2f;
		Alignment		mAlign = LEFT;
		GlslProgRef		mGlslProg;
	};

	// ---------------------------------------------------------------------------------------------

	class FontData;
	using FontDataRef = std::shared_ptr<FontData>;

	class Font {
	public:
		using Char = std::u32string::value_type;
		using Glyph = uint32_t;

		using CharToGlyphMap = std::unordered_map<SdfText::Font::Char, SdfText::Font::Glyph>;
		using GlyphToCharMap = std::unordered_map<SdfText::Font::Glyph, SdfText::Font::Char>;
		
		struct GlyphMetrics {
			vec2  advance;
			vec2  minimum;
			vec2  maximum;
		};

		using GlyphMeasure = vec2;

		struct GlyphInfo {
			uint32_t	mTextureIndex;
			Area		mTexCoords;
			vec2		mOriginOffset;
			vec2		mSize;
		};

		using GlyphMetricsMap = std::map<SdfText::Font::Glyph, SdfText::Font::GlyphMetrics>;
		using GlyphMeasuresList = std::vector<std::pair<SdfText::Font::Glyph, SdfText::Font::GlyphMeasure>>;
		using GlyphInfoMap = std::unordered_map<SdfText::Font::Glyph, SdfText::Font::GlyphInfo>;

		Font() {}
		Font( const std::string &name, float size );
		Font( DataSourceRef dataSource, float size );
		virtual ~Font();

		operator bool() const { return mData ? true : false; }

		float					getSize() const { return mSize; }

		const std::string&		getName() const { return mName; }
		std::string				getFullName() const { return mName; }

		float					getHeight() const { return mHeight; }
		float					getLeading() const { return mLeading; }
		float					getAscent() const { return mAscent; }
		float					getDescent() const { return mDescent; }

		size_t					getNumGlyphs() const { return mNumGlyphs; }
		Glyph					getGlyphIndex( size_t idx ) const;
		Glyph					getGlyphChar( char utf8Char ) const;
		std::vector<Glyph>		getGlyphs( const std::string &utf8Chars ) const;

		FT_Face					getFace() const;

		static const std::vector<std::string>&	getNames( bool forceRefresh = false );
		static SdfText::Font					getDefault();

	private:
		float					mSize = 0;
		std::string				mName;
		float					mHeight = 0;
		float					mLeading = 0;
		float					mAscent = 0;
		float					mDescent = 0;
		size_t					mNumGlyphs = 0;

		uint32_t				mUnitsPerEm = 0;

		FontDataRef				mData;
		void					loadFontData( const ci::DataSourceRef &dataSource );
		friend class SdfText;
	};

	// ---------------------------------------------------------------------------------------------

	struct CharPlacement {
		SdfText::Font::Glyph	mGlyph;
		Rectf					mSrcTexCoords;
		Rectf					mDstRect;
	};

	// ---------------------------------------------------------------------------------------------

	virtual ~SdfText();

	//! Creates a new SdfTextRef with font \a font, ensuring that glyphs necessary to render \a supportedChars are renderable, and format \a format
	static SdfTextRef		create( const SdfText::Font &font, const Format &format = Format(), const std::string &utf8Chars = SdfText::defaultChars() );
	//! Creates a new SdfTextRef with SDFT file at \a fontpath if it exists otherwise uses \a font and then saves SDFT file at \a filepath , ensuring that glyphs necessary to render \a supportedChars are renderable, and format \a format
	static SdfTextRef		create( const fs::path& filePath, const SdfText::Font &font, const Format &format = Format(), const std::string &utf8Chars = SdfText::defaultChars() );

	static void				save( const DataTargetRef& target, const SdfTextRef& sdfText );
	static void				save( const fs::path& filePath, const SdfTextRef& sdfText );
	static SdfTextRef		load( const DataSourceRef& source, float size = 0 );
	static SdfTextRef		load( const fs::path& filePath, float size = 0 );

	//! Draws string \a str at baseline \a baseline with DrawOptions \a options
	void	drawString( const std::string &str, const vec2 &baseline, const DrawOptions &options = DrawOptions() );
	//! Draws string \a str fit inside \a fitRect vertically, with internal offset \a offset and DrawOptions \a options
	void	drawString( const std::string &str, const Rectf &fitRect, const vec2 &offset = vec2(), const DrawOptions &options = DrawOptions() );
	//! Draws word-wrapped string \a str fit inside \a fitRect, with internal offset \a offset and DrawOptions \a options.
	void	drawStringWrapped( const std::string &str, const Rectf &fitRect, const vec2 &offset = vec2(), const DrawOptions &options = DrawOptions() );
	//! Draws the glyphs in \a glyphMeasures at baseline \a baseline with DrawOptions \a options. \a glyphMeasures is a vector of pairs of glyph indices and offsets for the glyph baselines
	void	drawGlyphs( const SdfText::Font::GlyphMeasuresList &glyphMeasures, const vec2 &baseline, const DrawOptions &options = DrawOptions(), const std::vector<ColorA8u> &colors = std::vector<ColorA8u>() );
	//! Draws the glyphs in \a glyphMeasures clipped by \a clip, with \a offset added to each of the glyph offsets with DrawOptions \a options. \a glyphMeasures is a vector of pairs of glyph indices and offsets for the glyph baselines.
	void	drawGlyphs( const SdfText::Font::GlyphMeasuresList &glyphMeasures, const Rectf &clip, vec2 offset, const DrawOptions &options = DrawOptions(), const std::vector<ColorA8u> &colors = std::vector<ColorA8u>() );

	//! Returns pairs of texture and final texture and vertex coords for drawing using \a glyphMeasures, \a baseline, and \a options.
	std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>>	placeChars( const SdfText::Font::GlyphMeasuresList &glyphMeasures, const vec2 &baseline, const DrawOptions &options = DrawOptions() );
	//! Returns pairs of texture and final texture and vertex coords for drawing using \a str, \a baseline, and \a options.
	std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>>	placeString( const std::string &str, const vec2 &baseline, const DrawOptions &options = DrawOptions() );
	//! Returns pairs of texture and final texture and vertex coords for wrapped drawing using \a str, \a fitRect,  \a offset, and \a options.
	std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>>	placeStringWrapped( const std::string &str, const Rectf &fitRect, const vec2 &offset = vec2(), const DrawOptions &options = DrawOptions() );
	
	//! Returns the bounds (as a Rectf) in pixels necessary to render the string \a str with DrawOptions \a options.
	Rectf	measureStringBounds( const std::string &str, const DrawOptions &options = DrawOptions() ) const;
	//! Returns the word-wrapped bounds (as a Rectf) in pixels necessary to render the string \a str with DrawOptions \a options.
	Rectf	measureStringBoundsWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options = DrawOptions() ) const;
	//! Returns the size in pixels necessary to render the string \a str with DrawOptions \a options.
	vec2	measureString( const std::string &str, const DrawOptions &options = DrawOptions() ) const;
	//! Returns the word-wrapped size in pixels necessary to render the string \a str with DrawOptions \a options.
	vec2	measureStringWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options = DrawOptions() ) const;
    
	//! Returns a vector of glyph/placement pairs representing \a str, suitable for use with drawGlyphs. Useful for caching placement and optimizing batching.
	std::vector<std::pair<SdfText::Font::Glyph,vec2>>		getGlyphPlacements( const std::string &str, const DrawOptions &options = DrawOptions() ) const;
	//! Returns a vector of glyph/placement pairs representing \a str fit inside \a fitRect, suitable for use with drawGlyphs. Useful for caching placement and optimizing batching.
	std::vector<std::pair<SdfText::Font::Glyph,vec2>>		getGlyphPlacements( const std::string &str, const Rectf &fitRect, const DrawOptions &options = DrawOptions() ) const;
	//! Returns a  word-wrapped vector of glyph/placement pairs representing \a str fit inside \a fitRect, suitable for use with drawGlyphs. Useful for caching placement and optimizing batching. Mac & iOS only.
	std::vector<std::pair<SdfText::Font::Glyph,vec2>>		getGlyphPlacementsWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options = DrawOptions() ) const;

	//! Returns the font the TextureFont represents
	const SdfText::Font&	getFont() const { return mFont; }
    //! Returns the name of the font
    std::string				getName() const { return mFont.getName(); }
	//! Returns the ascent of the font
	float					getAscent() const { return mFont.getAscent(); }
	//! Returns the descent of the font
	float					getDescent() const { return mFont.getDescent(); }

	//! Returns the default set of characters for a TextureFont, suitable for most English text, including some common ligatures and accented vowels.
	//! \c "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890().?!,:;'\"&*=+-/\\|@#_[]<>%^llflfiphridséáèà"
	static std::string		defaultChars();

	uint32_t				getNumTextures() const;
	const gl::TextureRef&	getTexture( uint32_t n ) const;

	const SdfText::Font::GlyphMetricsMap&	getGlyphMetrics() const { return mGlyphMetrics; }
	const SdfText::Font::CharToGlyphMap&	getCharToGlyph() const { return mCharToGlyph; }

	static gl::GlslProgRef	defaultShader();

private:
	SdfText( const SdfText::Font &font, const Format &format, const std::string &utf8Chars, bool generateSdf = true );
	friend class SdfTextManager;

	class TextureAtlas;
	using TextureAtlasRef = std::shared_ptr<TextureAtlas>;

	SdfText::Font						mFont;
	Format								mFormat;
	TextureAtlasRef						mTextureAtlases;
	SdfText::Font::GlyphMetricsMap		mGlyphMetrics;
	SdfText::Font::CharToGlyphMap		mCharToGlyph;
	SdfText::Font::GlyphToCharMap		mGlyphToChar;

	Rectf	measureStringImpl( const std::string &str, bool wrapped, const Rectf &fitRect, const DrawOptions &options ) const;
};

}} // namespace cinder::gl
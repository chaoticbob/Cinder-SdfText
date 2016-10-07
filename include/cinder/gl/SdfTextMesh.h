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

#include "cinder/gl/Batch.h"
#include "cinder/gl/SdfText.h"

namespace cinder { namespace gl {

class SdfTextMesh;
using SdfTextMeshRef = std::shared_ptr<SdfTextMesh>;

//! \class SdfTextMesh
//!
//!
class SdfTextMesh {
public:

	enum  Feature : uint32_t {
		NONE		= 0x00000000,
		TEXT		= 0x00000001,
		FONTSIZE	= 0x00000002,
		POSITION2	= 0x00000004,
		POSITION3	= 0x00000008,
		ROTATION	= 0x00000010,
		SCALE		= 0x00000020,
		WRAPPED		= 0x00000040,
		ALIGNMENT	= 0x00000080,
		JUSTIFY		= 0x00000100,
		ALL			= 0x7FFFFFFF
	};

	class Run;
	using RunRef = std::shared_ptr<Run>;

	//! \class Run
	//!
	//!
	class Run {
	public:
		//! \class Options
		//!
		//!
		class Options {
		public:
			Options() {}
			virtual ~Options() {}
			const vec3&					getPosition() const { return mPosition; }
			Options&					setPosition( const ci::vec2 &value ) { mPosition = vec3( value, 0.0f ); return *this; }
			Options&					setPosition( const ci::vec3 &value ) { mPosition = value; return *this; }
			float						getScale() const { return mScale; }
			Options&					setScale( float value ) { mScale = value; return *this; }
			const vec2&					getBaseline() const { return mBaseline; }
			Options&					setBaseline( const vec2 &value ) { mBaseline = value; return *this; }
			bool						getWrapped() const { return mWrapped; }
			Options&					setWrapped( bool value ) { mWrapped = value; if( ! mWrapped ) { mFitRect = Rectf( 0, 0, 0, 0 ); } return *this; }
			const Rectf&				getFitRect() const { return mFitRect; }
			Options&					setFitRect( const Rectf &value ) { mFitRect = value; mWrapped = true; return *this; }
			float						getLeading() const { return mDrawOptions.getLeading(); }
			Options&					setLeading( float value ) { mDrawOptions.leading( value ); return *this; }
			SdfText::Alignment			getAlignment() const { return mDrawOptions.getAlignment(); }
			Options&					setAlignment( SdfText::Alignment value ) { mDrawOptions.alignment( value ); return *this; }
			bool						getJustify() const { return mDrawOptions.getJustify(); }
			Options&					setJustify( bool value = true ) { mDrawOptions.justify( value ); return *this; }
			float						getDrawScale() const { return mDrawOptions.getScale(); }
			Options&					setDrawScale( float value ) { mDrawOptions.scale( value ); return *this; }
			const SdfText::DrawOptions&	getDrawOptions() const { return mDrawOptions; }
		private:
			friend class SdfTextMesh::Run;
			vec3						mPosition = vec3( 0 );
			quat						mRotation = quat( 1, vec3( 0 ) );
			float						mScale = 1.0f;
			vec2						mBaseline = vec2( 0 );
			bool						mWrapped = false;
			Rectf						mFitRect = Rectf( 0, 0, 0, 0 );
			SdfText::DrawOptions		mDrawOptions;
		};
		virtual ~Run() {}
		static RunRef				create( const std::string &utf8, const SdfTextRef &sdfText, const vec2& baseline, const Run::Options &drawOptions = Run::Options() );
		static RunRef				create( const std::string &utf8, const SdfText::Font &font, const vec2& baseline, const Run::Options &drawOptions = Run::Options(), const SdfText::Format &format = SdfText::Format(), const std::string &supportedUtf8Chars = SdfText::defaultChars() );
		static RunRef				create( const std::string &utf8, const SdfTextRef &sdfText, const Rectf fitRect, const Run::Options &drawOptions = Run::Options() );
		static RunRef				create( const std::string &utf8, const SdfText::Font &font, const Rectf fitRect, const Run::Options &drawOptions = Run::Options(), const SdfText::Format &format = SdfText::Format(), const std::string &supportedUtf8Chars = SdfText::defaultChars() );
		const SdfTextMesh*			getSdfTextMesh() const { return mSdfTextMesh; }
		const SdfTextRef&			getSdfText() const { return mSdfText; }
		uint32_t					getFeatures() const { return mFeatures; }
		void						enableFeature( Feature value ) { mFeatures |= value; }
		void						disableFeature( Feature value ) { mFeatures = ( mFeatures & ~value ); }
		uint32_t					getDirty() const { return mDirty; }
		void						setDirty( Feature value );
		void						clearDirty( Feature value) { mDirty = ( mDirty & ~value ); }
		const std::string&			getUtf8() const { return mUtf8; }
		const std::string&			getText() const { return getUtf8(); }
		void						setUtf8( const std::string &utf8 ) { if( utf8 != mUtf8 ) { mUtf8 = utf8; setDirty( Feature::TEXT ); } }
		void						setText( const std::string &utf8 ) { setUtf8( utf8 ); }
		const Run::Options&			getOptions() const { return mOptions; }
		const vec3&					getPosition() const { return mOptions.getPosition(); }
		void						setPosition( const ci::vec2 &value ) { mOptions.setPosition( value ); setDirty( Feature::POSITION2 ); clearDirty( Feature::POSITION3 ); }
		void						setPosition( const ci::vec3 &value ) { mOptions.setPosition( value ); setDirty( Feature::POSITION3 ); clearDirty( Feature::POSITION2 ); }
		float						getScale() const { return mOptions.getScale(); }
		void						setScale( float value ) { mOptions.setScale( value ); setDirty( Feature::SCALE ); }
		float						getLeading() const { return mOptions.getLeading(); }
		void						setLeading( float value ) { mOptions.setLeading( value ); }
		SdfText::Alignment			getAlignment() const { return mOptions.getAlignment(); }
		void						setAlignment( SdfText::Alignment value ) { mOptions.setAlignment( value ); setDirty( Feature::ALIGNMENT ); }
		bool						getJustify() const { return mOptions.getJustify(); }
		void						setJustify( bool value = true ) { mOptions.setJustify( value ); setDirty( Feature::JUSTIFY ); }
		const vec2&					getBaseline() const { return mOptions.getBaseline(); }
		void						setBaseline( const vec2 &value ) { mOptions.setBaseline( value ); }
		bool						getWrapped() const { return mOptions.getWrapped(); }
		void						setWrapped( bool value ) { mOptions.setWrapped( value ); }
		const Rectf&				getFitRect() const { return mOptions.getFitRect(); }
		void						setFitRect( const Rectf &value ) { mOptions.setFitRect( value ); }
		const Rectf&				getBounds() const { return mBounds; }
	private:
		Run( SdfTextMesh *sdfTextMesh, const std::string& utf8, const SdfTextRef& sdfText, const vec2 &baseline, const Run::Options &drawOptions );
		Run( SdfTextMesh *sdfTextMesh, const std::string& utf8, const SdfTextRef& sdfText, const Rectf &fitRect, const Run::Options &drawOptions );
		friend class SdfTextMesh;
		SdfTextMesh					*mSdfTextMesh = nullptr;
		SdfTextRef					mSdfText;
		uint32_t					mFeatures = Feature::TEXT;
		uint32_t					mDirty = Feature::NONE;
		std::string					mUtf8;
		Run::Options				mOptions;
		Rectf						mBounds = Rectf( 0, 0, 0, 0 );
	};

	virtual ~SdfTextMesh() {}

	static SdfTextMeshRef		create();

	void						appendText( const SdfTextMesh::RunRef &run );
	SdfTextMesh::RunRef			appendText( const std::string &utf8, const SdfTextRef &sdfText, const vec2& baseline, const Run::Options &options = Run::Options() );
	SdfTextMesh::RunRef			appendText( const std::string &utf8, const SdfText::Font &font, const vec2& baseline, const Run::Options &options = Run::Options() );
	SdfTextMesh::RunRef			appendTextWrapped( const std::string &utf8, const SdfTextRef &sdfText, const Rectf &fitRect, const Run::Options &options = Run::Options() );
	SdfTextMesh::RunRef			appendTextWrapped( const std::string &utf8, const SdfText::Font &font, const Rectf &fitRect, const Run::Options &options = Run::Options() );

	//! Returns the runs associated with \a sdfText. If \a sdfText is null, all runs gets returned.
	std::vector<RunRef>			getRuns( const SdfTextRef &sdfText = SdfTextRef() ) const;

	void						cache();

	void						draw( bool premultiply = true, float gamma = 2.2f );

	// NOT READY
	//void						draw( const SdfTextMesh::RunRef &run );

private:
	SdfTextMesh();

	struct TextBatch {
		VboRef					mIndexBuffer;
		VboRef					mVertexBuffer;
		BatchRef				mBatch;
		uint32_t				mIndexCount = 0;
	};

	using TextBatchMap = std::unordered_map<Texture2dRef, TextBatch>;

	struct RunDraw {
		uint32_t				mVertexStart = 0;
		uint32_t				mVertexCount = 0;
		TextBatchMap			mTextBatch;
	};

	struct TextDraw {
		uint32_t				mFeatures = Feature::TEXT;
		uint32_t				mDirty = Feature::NONE;
		TextBatchMap			mTextBatches;
	};

	using TextDrawRef = std::shared_ptr<TextDraw>;
	using RunMap = std::unordered_map<SdfTextRef, std::vector<RunRef>>;
	using TextDrawMap = std::unordered_map<SdfTextRef, TextDrawRef>;
	using RunDrawMap = std::unordered_map<RunRef, std::vector<RunDraw>>;

	bool						mDirty = false;
	RunMap						mRunMaps;
	TextDrawMap					mTextDrawMaps;
	RunDrawMap					mRunDrawMaps;

	void						updateFeatures( const Run *run );
	void						updateDirty( const Run *run );
};

}} // namespace cinder::gl
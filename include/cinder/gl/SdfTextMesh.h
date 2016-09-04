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
		POSITION	= 0x00000004,
		ROTATION	= 0x00000008,
		SCALE		= 0x00000010,
		BOUNDS		= 0x00000020,
		ALIGNMENT	= 0x00000040,
		JUSTIFY		= 0x00000080,
		ALL			= 0x7FFFFFFF
	};

	class Run;
	using RunRef = std::shared_ptr<Run>;

	//! \class Run
	//!
	//!
	class Run {
	public:
		//! \class DrawOptions
		//!
		//!
		class DrawOptions {
		public:
			DrawOptions() {}
			virtual ~DrawOptions() {}
			const vec3&			getPosition() const { return mPosition; }
			DrawOptions&		setPosition( const ci::vec2 &value ) { mPosition = vec3( value, 0.0f ); return *this; }
			DrawOptions&		setPosition( const ci::vec3 &value ) { mPosition = value; return *this; }
			float				getScale() const { return mScale; }
			DrawOptions&		setScale( float value ) { mScale = value; return *this; }
			SdfText::Alignment	getAlignment() const { return mAlignment; }
			DrawOptions&		setAlignment( SdfText::Alignment value ) { mAlignment = value; return *this; }
			bool				getJustify() const { return mJustify; }
			DrawOptions&		setJustify( bool value = true ) { mJustify = value; return *this; }
		private:
			friend class SdfTextMesh::Run;
			vec3				mPosition = vec3( 0 );
			quat				mRotation = quat( 1, vec3( 0 ) );
			float				mScale = 1.0f;
			Rectf				mBounds;
			SdfText::Alignment	mAlignment = SdfText::Alignment::LEFT;
			bool				mJustify = false;
		};
		virtual ~Run() {}
		static RunRef			create( const std::string &utf8, const SdfTextRef &sdfText, const Run::DrawOptions &drawOptions = Run::DrawOptions() );
		static RunRef			create( const std::string &utf8, const SdfText::Font &font, const Run::DrawOptions &drawOptions = Run::DrawOptions() );
		const SdfTextMesh*		getSdfTextMesh() const { return mSdfTextMesh; }
		const SdfTextRef&		getSdfText() const { return mSdfText; }
		uint32_t				getFeatures() const { return mFeatures; }
		void					enableFeature( Feature value ) { mFeatures |= value; }
		void					disableFeature( Feature value ) { mFeatures = ( mVertexStart & ~value ); }
		uint32_t				getDirty() const { return mDirty; }
		void					setDirty( Feature value = Feature::TEXT );
		void					clearDirty() { mDirty = Feature::NONE; }
		const std::string&		getUtf8() const { return mUtf8; }
		const std::string&		getText() const { return getUtf8(); }
		void					setUtf8( const std::string &utf8 ) { if( utf8 != mUtf8 ) { mUtf8 = utf8; setDirty( Feature::TEXT ); } }
		void					setText( const std::string &utf8 ) { setUtf8( utf8 ); }
		const vec3&				getPosition() const { return mDrawOptions.getPosition(); }
		void					setPosition( const ci::vec2 &value ) { mDrawOptions.setPosition( value ); setDirty( Feature::POSITION ); }
		void					setPosition( const ci::vec3 &value ) { mDrawOptions.setPosition( value ); setDirty( Feature::POSITION ); }
		float					getScale() const { return mDrawOptions.getScale(); }
		void					setScale( float value ) { mDrawOptions.setScale( value ); setDirty( Feature::SCALE ); }
		SdfText::Alignment		getAlignment() const { return mDrawOptions.getAlignment(); }
		void					setAlignment( SdfText::Alignment value ) { mDrawOptions.setAlignment( value ); setDirty( Feature::ALIGNMENT ); }
		bool					getJustify() const { return mDrawOptions.getJustify(); }
		void					setJustify( bool value = true ) { mDrawOptions.setJustify( value ); setDirty( Feature::JUSTIFY ); }
	private:
		Run( const std::string& utf8, const SdfTextRef& sdfText, SdfTextMesh *sdfTextMesh, const Run::DrawOptions &drawOptions );
		friend class SdfTextMesh;
		SdfTextMesh				*mSdfTextMesh = nullptr;
		SdfTextRef				mSdfText;
		uint32_t				mFeatures = Feature::TEXT;
		uint32_t				mDirty = Feature::NONE;
		std::string				mUtf8;
		Run::DrawOptions		mDrawOptions;
		uint32_t				mVertexStart = 0;
		uint32_t				mVertexCount = 0;
	};

	virtual ~SdfTextMesh() {}

	static SdfTextMeshRef		create();

	void						appendText( const SdfTextMesh::RunRef &run );
	SdfTextMesh::RunRef			appendText( const std::string &utf8, const SdfTextRef &sdfText, const Run::DrawOptions &drawOptions = Run::DrawOptions() );
	SdfTextMesh::RunRef			appendText( const std::string &utf8, const SdfText::Font &font, const Run::DrawOptions &drawOptions = Run::DrawOptions() );

	void						cache();

	void						draw();
	void						draw( const SdfTextMesh::RunRef &run );

private:
	SdfTextMesh();

	struct TextBatch {
		gl::Texture2dRef		mTexture;
		gl::BatchRef			mBatch;
	};

	struct RunDraw {
		uint32_t				mVertexStart = 0;
		uint32_t				mVertexCount = 0;
		TextBatch				mTextBatch;
	};

	struct TextDraw {
		uint32_t				mFeatures = Feature::TEXT;
		uint32_t				mDirty = Feature::NONE;
		std::vector<TextBatch>	mTextBatches;
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

	//void						draw( const TextDrawRef &textDraw );
};

}} // namespace cinder::gl
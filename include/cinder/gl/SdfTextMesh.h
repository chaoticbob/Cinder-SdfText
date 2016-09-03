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
		ALL			= 0x7FFFFFFF
	};

	class Run;
	using RunRef = std::shared_ptr<Run>;

	//! \class Run
	//!
	//!
	class Run {
	public:
		virtual ~Run() {}
		static RunRef		create( const std::string &utf8, const SdfTextRef &sdfText );
		static RunRef		create( const std::string &utf8, const SdfText::Font &font );
		void				enableFeature( Feature value ) { mFeatures |= value; }
		void				disableFeature( Feature value ) { mFeatures = ( mVertexStart & ~value ); }
		void				setDirty( Feature value = Feature::TEXT ) { mDirty |= value; }
		void				clearDirty() { mDirty = Feature::NONE; }
		const std::string&	getUtf8() const { return mUtf8; }
		const std::string&	getText() const { return getUtf8(); }
		void				setUtf8( const std::string &utf8 ) { if( utf8 != mUtf8 ) { mUtf8 = utf8; setDirty( Feature::TEXT ); } }
		void				setText( const std::string &utf8 ) { setUtf8( utf8 ); }
		void				setPosition( const ci::vec2 &position ) { mPosition = vec3( position, 0.0f ); setDirty( Feature::POSITION ); }
		void				setPosition( const ci::vec3 &position ) { mPosition = position; setDirty( Feature::POSITION ); }
		void				setScale( const ci::vec2 &scale ) { mScale = scale; setDirty( Feature::SCALE ); }
	private:
		Run( const std::string& utf8, const SdfTextRef& sdfText, SdfTextMesh *sdfTextMesh );
		friend class SdfTextMesh;
		SdfTextMesh			*mSdfTextMesh = nullptr;
		SdfTextRef			mSdfText;
		uint32_t			mFeatures = Feature::TEXT;
		uint32_t			mDirty = Feature::NONE;
		std::string			mUtf8;
		float				mFontSize = -1.0f;
		Rectf				mBounds;
		vec3				mPosition = vec3( 0 );
		quat				mRotation = quat( 1, vec3( 0 ) );
		vec2				mScale = vec2( 1 );
		uint32_t			mVertexStart = 0;
		uint32_t			mVertexCount = 0;
	};

	virtual ~SdfTextMesh() {}

	static SdfTextMeshRef		create();

	SdfTextMesh::RunRef			appendText( const std::string &utf8, const SdfTextRef &sdfText );
	SdfTextMesh::RunRef			appendText( const std::string &utf8, const SdfText::Font &font );
	void						appendText( const SdfTextMesh::RunRef &run );

	void						cache();

	void						draw();
	void						draw( const SdfTextMesh::RunRef &run );

private:
	SdfTextMesh();

	Feature						mFeatures = Feature::TEXT;
	std::vector<RunRef>			mRuns;
	std::vector<Texture2dRef>	mTextures;

	// Texture index and batch map
	using CharsBatch = std::pair<uint32_t, BatchRef>;
	std::vector<CharsBatch>		mBatches;
};

}} // namespace cinder::gl
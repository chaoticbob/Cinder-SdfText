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

#include "cinder/gl/SdfTextMesh.h"
#include "cinder/gl/scoped.h"

namespace cinder { namespace gl {

// -------------------------------------------------------------------------------------------------
// SdfTextMesh::Run
// -------------------------------------------------------------------------------------------------
SdfTextMesh::Run::Run( const std::string& utf8, const gl::SdfTextRef& sdfText, SdfTextMesh *sdfTextMesh, const Run::DrawOptions &drawOptions )
	: mUtf8( utf8 ),
	  mSdfText( sdfText ), 
	  mSdfTextMesh( sdfTextMesh ), 
	  mDrawOptions( drawOptions )
{
}

void SdfTextMesh::Run::setDirty( Feature value )
{
	mDirty |= value;
	if( nullptr != mSdfTextMesh ) {
		mSdfTextMesh->updateDirty( this );
	}
}

// -------------------------------------------------------------------------------------------------
// SdfTextMesh
// -------------------------------------------------------------------------------------------------
SdfTextMesh::SdfTextMesh()
{
}

SdfTextMeshRef SdfTextMesh::create()
{
	SdfTextMeshRef result = SdfTextMeshRef( new SdfTextMesh() );
	return result;
}

SdfTextMesh::RunRef SdfTextMesh::appendText( const std::string &utf8, const SdfTextRef &sdfText, const Run::DrawOptions &drawOptions )
{
	SdfTextMesh::RunRef run = SdfTextMesh::RunRef( new SdfTextMesh::Run( utf8, sdfText, this, drawOptions ) );
	appendText( run );
	return run;
}

SdfTextMesh::RunRef SdfTextMesh::appendText( const std::string &utf8, const SdfText::Font &font, const Run::DrawOptions &drawOptions )
{
	SdfTextRef sdfText = SdfText::create( font );
	SdfTextMesh::RunRef run = SdfTextMesh::RunRef( new SdfTextMesh::Run( utf8, sdfText, this, drawOptions ) );
	appendText( run );
	return run;
}

void SdfTextMesh::appendText( const SdfTextMesh::RunRef &run )
{
	const auto& sdfText = run->getSdfText();
	if( ! sdfText ) {
		throw ci::Exception( "run does not contain valid gl::SdfText" );
	}

	auto& runs = mRunMaps[sdfText];

	// Bail if the run already exists
	auto runIt = std::find_if( std::begin( runs ), std::end( runs ),
		[run]( const RunRef &elem ) -> bool {
			return elem == run;
		}
	);
	if( std::end( runs ) != runIt ) {
		return;
	}

	// Add run
	runs.push_back( run );

	// Check and add draw info if necessary
	auto drawIt = mTextDrawMaps.find( sdfText );
	if( mTextDrawMaps.end() == drawIt ) {
		mTextDrawMaps[sdfText] = SdfTextMesh::TextDrawRef( new SdfTextMesh::TextDraw() );
	}

	// Map run to text draw
	auto& textDraw = mTextDrawMaps[sdfText];
	mRunDrawMaps[run] = textDraw;

	// Update flags
	updateFeatures( run.get() );
	updateDirty( run.get() );
}

void SdfTextMesh::updateFeatures( const Run *run )
{
	const auto& sdfText = run->getSdfText();
	auto drawIt = mTextDrawMaps.find( sdfText );
	if( mTextDrawMaps.end() != drawIt ) {
		auto& textDraw = drawIt->second;
		textDraw->mFeatures |= run->getFeatures();
	}
}

void SdfTextMesh::updateDirty( const Run *run )
{
	const auto& sdfText = run->getSdfText();
	auto drawIt = mTextDrawMaps.find( sdfText );
	if( mTextDrawMaps.end() != drawIt ) {
		auto& textDraw = drawIt->second;
		textDraw->mDirty |= run->getDirty();
		mDirty = true;
	}
}

void SdfTextMesh::cache()
{
	if( ! mDirty ) {
		return;
	}

	for( auto& runMapIt : mRunMaps ) {
		auto& sdfText = runMapIt.first;
		auto& runs = runMapIt.second;
	}

	mDirty = false;
}

//void SdfTextMesh::draw( const TextDrawRef &textDraw )
//{
//	if( ! textDraw ) {
//		return;
//	}
//
//	for( const auto& batchIt : textDraw->mBatches ) {
//		auto& tex = batchIt.mTexture;
//		auto& batch = batchIt.mBatch;
//		ScopedTextureBind scopedTexture( tex );
//		batch->draw();
//	}
//}

void SdfTextMesh::draw()
{
	cache();

	for( const auto& textDrawIt : mTextDrawMaps ) {
		const auto& textDraw = textDrawIt.second;
	}
}

void SdfTextMesh::draw( const SdfTextMesh::RunRef &run )
{
	auto it = mRunDrawMaps.find( run );
	if( mRunDrawMaps.end() == it ) {
		return;
	}

	cache();

	auto& runDraws = mRunDrawMaps[run];
	for( auto &runDrawIt : runDraws ) {
	}
}

}} // namespace cinder::gl
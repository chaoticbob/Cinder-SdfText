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
#include "cinder/gl/Context.h"
#include "cinder/gl/scoped.h"
#include "cinder/TriMesh.h"

namespace cinder { namespace gl {

// -------------------------------------------------------------------------------------------------
// SdfTextMesh::Run
// -------------------------------------------------------------------------------------------------
SdfTextMesh::Run::Run( SdfTextMesh *sdfTextMesh, const std::string& utf8, const gl::SdfTextRef& sdfText, const vec2 &baseline, const Run::Options &drawOptions )
	: mSdfTextMesh( sdfTextMesh ),
	  mUtf8( utf8 ),
	  mSdfText( sdfText ), 
	  mOptions( drawOptions )
{
	mOptions.setBaseline( baseline );
}

SdfTextMesh::Run::Run( SdfTextMesh *sdfTextMesh, const std::string& utf8, const gl::SdfTextRef& sdfText, const Rectf &fitRect, const Run::Options &drawOptions )
	: mSdfTextMesh( sdfTextMesh ),
	  mUtf8( utf8 ),
	  mSdfText( sdfText ), 
	  mOptions( drawOptions )
{
	mOptions.setFitRect( fitRect );
}

SdfTextMesh::RunRef SdfTextMesh::Run::create( const std::string &utf8, const SdfTextRef &sdfText, const vec2& baseline, const Run::Options &drawOptions )
{
	SdfTextMesh::RunRef result = SdfTextMesh::RunRef( new SdfTextMesh::Run( nullptr, utf8, sdfText, baseline, drawOptions ) );
	return result;
}

SdfTextMesh::RunRef SdfTextMesh::Run::create( const std::string &utf8, const SdfText::Font &font, const vec2& baseline, const Run::Options &drawOptions, const SdfText::Format &format, const std::string &supportedUtf8Chars )
{
	auto sdfText = SdfText::create( font, format, supportedUtf8Chars );
	SdfTextMesh::RunRef result = SdfTextMesh::RunRef( new SdfTextMesh::Run( nullptr, utf8, sdfText, baseline, drawOptions ) );
	return result;
}

SdfTextMesh::RunRef SdfTextMesh::Run::create( const std::string &utf8, const SdfTextRef &sdfText, const Rectf fitRect, const Run::Options &drawOptions )
{
	SdfTextMesh::RunRef result = SdfTextMesh::RunRef( new SdfTextMesh::Run( nullptr, utf8, sdfText, fitRect, drawOptions ) );
	return result;
}

SdfTextMesh::RunRef SdfTextMesh::Run::create( const std::string &utf8, const SdfText::Font &font, const Rectf fitRect, const Run::Options &drawOptions, const SdfText::Format &format, const std::string &supportedUtf8Chars )
{
	auto sdfText = SdfText::create( font, format, supportedUtf8Chars );
	SdfTextMesh::RunRef result = SdfTextMesh::RunRef( new SdfTextMesh::Run( nullptr, utf8, sdfText, fitRect, drawOptions ) );
	return result;
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

SdfTextMesh::RunRef SdfTextMesh::appendText( const std::string &utf8, const SdfTextRef &sdfText, const vec2& baseline, const Run::Options &options )
{
	SdfTextMesh::RunRef run = SdfTextMesh::RunRef( new SdfTextMesh::Run( this, utf8, sdfText, baseline, options ) );
	appendText( run );
	return run;
}

SdfTextMesh::RunRef SdfTextMesh::appendText( const std::string &utf8, const SdfText::Font &font, const vec2& baseline, const Run::Options &options )
{
	SdfTextRef sdfText = SdfText::create( font );
	SdfTextMesh::RunRef run = SdfTextMesh::RunRef( new SdfTextMesh::Run( this, utf8, sdfText, baseline, options ) );
	appendText( run );
	return run;
}

SdfTextMesh::RunRef SdfTextMesh::appendTextWrapped( const std::string &utf8, const SdfTextRef &sdfText, const Rectf &fitRect, const Run::Options &options )
{
	SdfTextMesh::RunRef run = SdfTextMesh::RunRef( new SdfTextMesh::Run( this, utf8, sdfText, fitRect, options ) );
	appendText( run );
	return run;
}

SdfTextMesh::RunRef SdfTextMesh::appendTextWrapped( const std::string &utf8, const SdfText::Font &font, const Rectf &fitRect, const Run::Options &options )
{
	SdfTextRef sdfText = SdfText::create( font );
	SdfTextMesh::RunRef run = SdfTextMesh::RunRef( new SdfTextMesh::Run( this, utf8, sdfText, fitRect, options ) );
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
	run->mSdfTextMesh = this;
	runs.push_back( run );

	// Check and add text draw info if necessary
	auto textDrawIt = mTextDrawMaps.find( sdfText );
	if( mTextDrawMaps.end() == textDrawIt ) {
		mTextDrawMaps[sdfText] = SdfTextMesh::TextDrawRef( new SdfTextMesh::TextDraw() );
	}

	// Check and add run draw info if necessary
	auto runDrawIt = mRunDrawMaps.find( run );
	if( mRunDrawMaps.end() == runDrawIt ) {
		mRunDrawMaps[run] = std::vector<RunDraw>();
	}

	// Update flags
	updateFeatures( run.get() );
	updateDirty( run.get() );
}

std::vector<SdfTextMesh::RunRef> SdfTextMesh::getRuns( const SdfTextRef &sdfText ) const
{
	std::vector<SdfTextMesh::RunRef> result;
	if( sdfText ) {
		auto it = mRunMaps.find( sdfText );
		if( mRunMaps.end() != it ) {
			result = it->second;
		}
	}
	else {
		for( const auto &it : mRunMaps ) {
			std::copy( std::begin( it.second ), std::end( it.second ), std::back_inserter( result ) );
		}
	}
	return result;
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

struct ClientMesh {
	struct Tri {
		uint32_t	v0;
		uint32_t	v1;
		uint32_t	v2;
	};
	
	struct Vertex {
		vec4 pos;
		vec2 uv;
	};

	std::vector<Tri>		mTriangles;
	std::vector<Vertex>		mVertices;

	uint32_t getNumTriangles() const {
		return static_cast<uint32_t>( mTriangles.size() );
	}

	uint32_t getNumIndices() const { 
		uint32_t ntri = getNumTriangles();
		uint32_t result = 3 * ntri;
		return result;
	}

	uint32_t getNumVertices() const {
		return static_cast<uint32_t>( mVertices.size() );
	}

	void appendVertex( const vec2 &pos, const vec2 &uv ) {
		mVertices.push_back( { vec4( pos.x, pos.y, 0.0f, 1.0f ), uv } );
	}

	void appendTriangle( uint32_t v0, uint32_t v1, uint32_t v2 ) { 
		mTriangles.push_back( { v0, v1, v2 } );
	}

	const Tri *getTrianglesData() const {
		return mTriangles.data();
	}

	const uint32_t *getIndicesData() const {
		return reinterpret_cast<const uint32_t *>( getTrianglesData() );
	}

	const Vertex *getVerticesData() const {
		return mVertices.data();
	}
};

void SdfTextMesh::cache()
{
	if( ! mDirty ) {
		return;
	}

	for( auto &runMapIt : mRunMaps ) {
		auto &sdfText = runMapIt.first;
		auto &runs = runMapIt.second;

		// Get textures
		std::vector<Texture2dRef> textures;
		{
			uint32_t numTextures = sdfText->getNumTextures();
			for( uint32_t i = 0; i < numTextures; ++i ) {
				auto tex = sdfText->getTexture( i );
				textures.push_back( sdfText->getTexture( i ) );
			}
		}

		std::unordered_map<RunRef, std::pair<uint32_t, uint32_t>> runVertRanges;
		std::unordered_map<Texture2dRef, ClientMesh> texToMesh;
		for( const auto &run : runs ) {
			std::pair<uint32_t, uint32_t> vertRange = std::make_pair( 0, 0 );
			const auto &options = run->getOptions();	
			auto &bounds = run->mBounds;
			std::vector<std::pair<uint8_t, std::vector<SdfText::CharPlacement>>> placements; 
			if( run->getWrapped() ) {
				const Rectf &fitRect = run->getFitRect();
				vec2 offset = vec2( run->getPosition() );
				placements = sdfText->placeStringWrapped( run->getUtf8(), fitRect, offset, options.getDrawOptions() );
				bounds = sdfText->measureStringBoundsWrapped( run->getUtf8(), fitRect, options.getDrawOptions() );
				bounds += vec2( fitRect.x1, fitRect.y1 );
				bounds += offset;
			}
			else {
				vec2 baseline = vec2( run->getBaseline() );
				placements = sdfText->placeString( run->getUtf8(), baseline, options.getDrawOptions() );
				bounds = sdfText->measureStringBounds( run->getUtf8(),options.getDrawOptions() );
				bounds += baseline;
			}
			for( const auto &placementsIt : placements ) {
				Texture2dRef tex = textures[placementsIt.first];
				const auto& charPlacements = placementsIt.second;
				if( charPlacements.empty() ) {
					continue;
				}

				auto &mesh = texToMesh[tex];
				vertRange.first = static_cast<uint32_t>( mesh.getNumIndices() );
				for( const auto& place : charPlacements ) {
					const auto& srcTexCoords = place.mSrcTexCoords;
					const auto& destRect = place.mDstRect;
					vec2 P0 = vec2( destRect.getX2(), destRect.getY1() );
					vec2 P1 = vec2( destRect.getX1(), destRect.getY1() );
					vec2 P2 = vec2( destRect.getX2(), destRect.getY2() );
					vec2 P3 = vec2( destRect.getX1(), destRect.getY2() );
					vec2 uv0 = vec2( srcTexCoords.getX2(), srcTexCoords.getY1() );
					vec2 uv1 = vec2( srcTexCoords.getX1(), srcTexCoords.getY1() );
					vec2 uv2 = vec2( srcTexCoords.getX2(), srcTexCoords.getY2() );
					vec2 uv3 = vec2( srcTexCoords.getX1(), srcTexCoords.getY2() );
					mesh.appendVertex( P0, uv0 );
					mesh.appendVertex( P1, uv1 );
					mesh.appendVertex( P2, uv2 );
					mesh.appendVertex( P3, uv3 );
			
					uint32_t nverts = static_cast<uint32_t>( mesh.getNumVertices() );
					uint32_t v0 = nverts - 4;
					uint32_t v1 = nverts - 3;
					uint32_t v2 = nverts - 2;
					uint32_t v3 = nverts - 1;
					mesh.appendTriangle( v0, v1, v2 );
					mesh.appendTriangle( v2, v1, v3 );
				}
				vertRange.second = static_cast<uint32_t>( mesh.getNumIndices() );
				runVertRanges[run] = vertRange;
			}
		}

		auto& textDraws = mTextDrawMaps[sdfText];
		for( const auto& tmIt : texToMesh ) {
			auto& tex = tmIt.first;
			auto& mesh = tmIt.second;
			auto& textBatch = textDraws->mTextBatches[tex];

			if( ! textBatch.mBatch ) {
				// Create index buffer
				textBatch.mIndexBuffer = Vbo::create( GL_ELEMENT_ARRAY_BUFFER );
				// Create vertex layout
				auto vertexLayout = geom::BufferLayout();
				vertexLayout.append( geom::POSITION,    4, sizeof( ClientMesh::Vertex ), static_cast<size_t>( offsetof( ClientMesh::Vertex, pos ) ) );
				vertexLayout.append( geom::TEX_COORD_0, 2, sizeof( ClientMesh::Vertex ), static_cast<size_t>( offsetof( ClientMesh::Vertex, uv  ) ) );
				// Create Vertex buffer
				textBatch.mVertexBuffer = Vbo::create( GL_ARRAY_BUFFER );
				// Create vbo mesh - index count is passed in to prevent data corruption on NVIDIA cards
				VboMeshRef vboMesh = VboMesh::create( 0, GL_TRIANGLES, { std::make_pair( vertexLayout, textBatch.mVertexBuffer  ) }, mesh.getNumIndices(), GL_UNSIGNED_INT, textBatch.mIndexBuffer );
				// Create batch using vbo mesh and default SdfText sahder
				textBatch.mBatch = Batch::create( vboMesh, SdfText::defaultShader() );
			}

			// Buffer index and vertex data
			textBatch.mIndexBuffer->bufferData( sizeof( uint32_t ) * mesh.getNumIndices(), mesh.getIndicesData(), GL_STATIC_DRAW );
			textBatch.mVertexBuffer->bufferData( sizeof( ClientMesh::Vertex ) * mesh.getNumVertices(), mesh.getVerticesData(), GL_STATIC_DRAW );
			// Update Index count
			textBatch.mIndexCount = mesh.getNumIndices();
		}
	}

	mDirty = false;
}

void SdfTextMesh::draw( bool premultiply, float gamma )
{
	cache();

	for( auto& textDrawIt : mTextDrawMaps ) {
		auto& textDraw = textDrawIt.second;
		for( auto& textBatchIt : textDraw->mTextBatches ) {
			auto& tex = textBatchIt.first;
			auto& textBatch = textBatchIt.second;
			auto& batch = textBatch.mBatch;
			auto& shader = batch->getGlslProg();


			ScopedTextureBind scopedTexture( tex, 0 );
			shader->uniform( "uTex0", 0 );

			shader->uniform( "uFgColor", gl::context()->getCurrentColor() );
			shader->uniform( "uPremultiply", premultiply ? 1.0f : 0.0f );
			shader->uniform( "uGamma", gamma );


			batch->draw( 0, textBatch.mIndexCount );
		}
	}
}

// NOTE READY
/*
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
*/

}} // namespace cinder::gl
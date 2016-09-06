#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//! \class FontSizesApp
//!
//!
class FontSizesApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	struct TextInfo {
		std::string			mName;
		float				mSize;
		vec2				mBaseline;
		gl::SdfText::Font	mFont;
		gl::SdfTextRef		mSdfText;
		float				mDrawScale;
		TextInfo() {}
		TextInfo( const std::string &name, float size, const vec2 &baseline, const float drawScale = 1.0f ) 
			: mName( name ), mSize( size ), mBaseline( baseline ), mDrawScale( drawScale ) {}
	};

	std::vector<TextInfo>	mTextInfos;

	void generateSdf();

	gl::TextureRef			mTex;
	bool					mPremultiply = false;
};

void FontSizesApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../FontSizes/assets" );
#endif

	mTex = gl::Texture::create( loadImage( getAssetPath( "bg.png" ) ) );

	mTextInfos.push_back( TextInfo( "Arial", 6.0f, vec2( 10,  64 + 1*50 ), 8.0f ) );
	mTextInfos.push_back( TextInfo( "Arial", 16.0f, vec2( 10,  64 + 2*50 ), 3.0f ) );
	
	mTextInfos.push_back( TextInfo( "Cambria", 6.0f, vec2( 10, 256 + 1*50 ), 8.0f ) );
	mTextInfos.push_back( TextInfo( "Cambria", 12.0f, vec2( 10, 256 + 2*50 ), 4.0f ) );
	
	mTextInfos.push_back( TextInfo( "Impact", 6.0f, vec2( 10, 448 + 1*50 ), 8.0f ) );
	mTextInfos.push_back( TextInfo( "Impact", 24.0f, vec2( 10, 448 + 2*50 ), 2.0f ) );

	mTextInfos.push_back( TextInfo( "Showcard Gothic", 4.0f, vec2( 10, 640 + 1*50 ), 10.0f ) );
	mTextInfos.push_back( TextInfo( "Showcard Gothic", 40.0f, vec2( 10, 640 + 2*50 ), 1.0f ) );

	mTextInfos.push_back( TextInfo( "Times New Roman", 4.0f, vec2( 10, 832 + 1*50 ), 14.0f ) );
	mTextInfos.push_back( TextInfo( "Times New Roman", 112.0f, vec2( 10, 832 + 2*50 ), 0.5f ) );

	generateSdf();
}

void FontSizesApp::generateSdf()
{
	int i = 0;
	for( auto &ti : mTextInfos ) {
		fs::path cacheFile = getAssetPath( "" ) / ( "cached_font_" + toString( i ) + ".sdft" );
		ti.mFont = gl::SdfText::Font( ti.mName, ti.mSize );
		ti.mSdfText = gl::SdfText::create( cacheFile, ti.mFont, gl::SdfText::Format().sdfScale( 2.0f ) );
		++i;
	}
}

void FontSizesApp::keyDown( KeyEvent event )
{
}

void FontSizesApp::mouseDown( MouseEvent event )
{
	mPremultiply = ! mPremultiply;
}

void FontSizesApp::update()
{
}

void FontSizesApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );

	gl::color( 1, 1, 1 );
	gl::draw( mTex, mTex->getBounds() );
	
	const std::string str = "Five boxing wizards";

	gl::color( 0.45f, 0.45f, 0 );
	float y = 64;
	for( int i = 0; i < 5; ++i ) {
		gl::drawLine( vec2( 0, y ), vec2( getWindowWidth(), y ) );
		y += 192;
	}
	gl::drawLine( vec2( 10, 0 ), vec2( 10, getWindowHeight() ) );

	for( const auto &ti : mTextInfos ) {
		gl::color( 0.45f, 0.45f, 0 );
		gl::drawLine( vec2( 0, ti.mBaseline.y ), vec2( getWindowWidth(), ti.mBaseline.y ) );

		{
			{
				gl::ScopedModelMatrix scopedModel;
				gl::translate( ti.mBaseline );
				gl::scale( vec2( ti.mDrawScale ) );

				gl::color( 1, 1, 1 );
				ti.mSdfText->drawString( str, vec2( 0 ), gl::SdfText::DrawOptions().scale( 1.0f ).premultiply( mPremultiply ) );
			}

			{
				gl::ScopedModelMatrix scopedModel;
				gl::translate( ti.mBaseline + vec2( 470, 0 ) );
				gl::scale( vec2( 0.4f * ti.mDrawScale ) );

				gl::color( 0.1f, 1, 1 );
				std::stringstream ss;
				ss << ti.mFont.getSize() << "pt @ " << (int)(ti.mDrawScale*100.0f) << "%";
				ti.mSdfText->drawString( ss.str(), vec2( 0 ), gl::SdfText::DrawOptions().scale( 1.0f ).premultiply( mPremultiply ) );
			}
		}
	}
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize( 640, 1024 );
}

CINDER_APP( FontSizesApp, RendererGl, prepareSettings );

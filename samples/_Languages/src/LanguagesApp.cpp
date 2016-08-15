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

//! \class LanguagesApp
//!
//!
class LanguagesApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	struct TextInfo {
		std::string			mName;
		std::string			mStr;
		float				mSize;
		vec2				mBaseline;
		gl::SdfText::Font	mFont;
		gl::SdfTextRef		mSdfText;
		float				mDrawScale;
		TextInfo() {}
		TextInfo( const std::string &name, const std::string &str, float size, const vec2 &baseline, const float drawScale = 1.0f ) 
			: mName( name ), mStr( str ), mSize( size ), mBaseline( baseline ), mDrawScale( drawScale ) {}
	};

	std::vector<TextInfo>	mTextInfos;

	void generateSdf();
};

void LanguagesApp::setup()
{
#if defined( CINDER_MSW )
	mTextInfos.push_back( TextInfo( "Arial", "English", 42.0f, vec2( 10,  50 + 0*50 ) ) );
	mTextInfos.push_back( TextInfo( "Meiryo", "日本語", 42.0f, vec2( 10,  50 + 1*50 ) ) );
	mTextInfos.push_back( TextInfo( "Dengxian", "中文", 42.0f, vec2( 10,  50 + 2*50 ) ) );
	mTextInfos.push_back( TextInfo( "Malgun Gothic", "한국어", 42.0f, vec2( 10,  50 + 3*50 ) ) );
	mTextInfos.push_back( TextInfo( "Calibri", "Tiếng Việt", 42.0f, vec2( 10,  50 + 4*50 ) ) );
	mTextInfos.push_back( TextInfo( "Leelawadee", "ไทย", 42.0f, vec2( 10,  50 + 5*50 ) ) );
	mTextInfos.push_back( TextInfo( "Impact", "ελληνικά", 42.0f, vec2( 10,  50 + 6*50 ) ) );
#endif

	generateSdf();
}

void LanguagesApp::generateSdf()
{
	for( auto &ti : mTextInfos ) {
		ti.mFont = gl::SdfText::Font( ti.mName, ti.mSize );
		ti.mSdfText = gl::SdfText::create( ti.mFont, gl::SdfText::Format(), ti.mStr );
	}
}

void LanguagesApp::keyDown( KeyEvent event )
{
}

void LanguagesApp::mouseDown( MouseEvent event )
{
}

void LanguagesApp::update()
{
}

void LanguagesApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );

	for( const auto &ti : mTextInfos ) {
		gl::color( 0.45f, 0.45f, 0 );
		gl::drawLine( vec2( 0, ti.mBaseline.y ), vec2( getWindowWidth(), ti.mBaseline.y ) );

		gl::color( 1, 1, 1 );
		ti.mSdfText->drawString( ti.mStr, ti.mBaseline );
	}
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize( 720, 540 );
}

CINDER_APP( LanguagesApp, RendererGl, prepareSettings );

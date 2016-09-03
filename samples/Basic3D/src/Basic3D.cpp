#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//! \class Basic3DApp
//!
//!
class Basic3DApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::SdfText::Font	mFont;
	gl::SdfTextRef		mSdfText;
	bool				mPremultiply = false;
};

void Basic3DApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../Basic3D/assets" );
#endif

	// Select original font
	mFont = gl::SdfText::Font( loadAsset( "VarelaRound-Regular.ttf" ), 24 );

	// If cached_font.sdft exists, it will be loaded.
	// If cached_font.sdft does not exist, it will be generated and then loaded.
	mSdfText = gl::SdfText::create( getAssetPath( "" ) / "cached_font.sdft", mFont );

	// Make sure we're looking at the loaded version of the font object and not the original
	mFont = mSdfText->getFont();
}

void Basic3DApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '=':
		case '+':
			mSdfText = gl::SdfText::load( getAssetPath( "cached_font.sdft" ), mFont.getSize() + 1 );
			mFont = mSdfText->getFont();
		break;
		case '-':
			mSdfText = gl::SdfText::load( getAssetPath( "cached_font.sdft" ), mFont.getSize() - 1 );
			mFont = mSdfText->getFont();
		break;
		case 'p':
		case 'P':
			mPremultiply = ! mPremultiply;
		break;
	}
}

void Basic3DApp::mouseDown( MouseEvent event )
{
}

void Basic3DApp::update()
{
}

void Basic3DApp::draw()
{
	gl::setMatricesWindowPersp( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::ScopedMatrices scopedMatrices;
	gl::translate( vec2( getWindowSize() ) / 2.0f );
	gl::rotate( 0.8f * getElapsedSeconds(), vec3( 1, 0, 0 ) );
	gl::scale( vec3( 8.0f ) );

	gl::color( Color::white() );
	mSdfText->drawString( "Hello", vec2( 0 ) );

	/*
	std::string str( "Granted, then, that certain transformations do happen, it is essential that we should regard them in the philosophic manner of fairy tales, not in the unphilosophic manner of science and the \"Laws of Nature.\" When we are asked why eggs turn into birds or fruits fall in autumn, we must answer exactly as the fairy godmother would answer if Cinderella asked her why mice turned into horses or her clothes fell from her at twelve o'clock. We must answer that it is MAGIC. It is not a \"law,\" for we do not understand its general formula." );
	Rectf boundsRect( 40, mSdfText->getAscent() + 50, getWindowWidth() - 40, getWindowHeight() - 40 );

	gl::color( ColorA( 1, 0.5f, 0.25f, 1.0f ) );

	auto drawOptions = gl::SdfText::DrawOptions().premultiply( mPremultiply );

	mSdfText->drawStringWrapped( str, boundsRect, vec2( 0 ), drawOptions );

	// Draw FPS
	gl::color( Color::white() );
	mSdfText->drawString( toString( floor( getAverageFps() ) ) + " FPS | " + std::string( mPremultiply ? "premult" : "" ), vec2( 10, getWindowHeight() - mSdfText->getDescent() ), drawOptions );
    
    // Draw Font Name
	float fontNameWidth = mSdfText->measureString( mSdfText->getName() ).x;
	mSdfText->drawString( mSdfText->getName(), vec2( getWindowWidth() - fontNameWidth - 10, getWindowHeight() - mSdfText->getDescent() ), drawOptions );
	*/
}

void prepareSettings( App::Settings *settings )
{
}

CINDER_APP( Basic3DApp, RendererGl, prepareSettings );

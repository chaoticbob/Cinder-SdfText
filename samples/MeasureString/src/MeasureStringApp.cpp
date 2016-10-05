#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//! \class MeasureStringApp
//!
//!
class MeasureStringApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::SdfText::Font		mFont;
	gl::SdfTextRef			mSdfText;
	bool					mPremultiply = false;
	gl::SdfText::Alignment	mAlignment = gl::SdfText::Alignment::LEFT;
	bool					mJustify = false;

	void drawMeasuredStringRect( const std::string &str, const vec2 &baseline, const gl::SdfTextRef &sdfText, const gl::SdfText::DrawOptions &drawOptions );
};

void MeasureStringApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../MeasureString/assets" );
#endif

	mFont = gl::SdfText::Font( loadAsset( "fonts/VarelaRound-Regular.ttf" ), 24 );
	mSdfText = gl::SdfText::create( getAssetPath( "" ) / "cached_font.sdft", mFont );
}

void MeasureStringApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '=':
		case '+':
			mFont = gl::SdfText::Font( loadAsset( "fonts/VarelaRound-Regular.ttf" ), mFont.getSize() + 1 );
			mSdfText = gl::SdfText::create( getAssetPath( "" ) / "cached_font.sdft", mFont );
		break;
		case '-':
			mFont = gl::SdfText::Font( loadAsset( "fonts/VarelaRound-Regular.ttf" ), mFont.getSize() - 1 );
			mSdfText = gl::SdfText::create( getAssetPath( "" ) / "cached_font.sdft", mFont );
		break;
		case 'p':
		case 'P':
			mPremultiply = ! mPremultiply;
		break;
		case 'l':
		case 'L':
			mAlignment = gl::SdfText::Alignment::LEFT;
		break;
		case 'r':
		case 'R':
			mAlignment = gl::SdfText::Alignment::RIGHT;
		break;
		case 'c':
		case 'C':
			mAlignment = gl::SdfText::Alignment::CENTER;
		break;
		case 'j':
		case 'J':
			mJustify = ! mJustify;
		break;
	}
}

void MeasureStringApp::mouseDown( MouseEvent event )
{
	// NOTE: This may take a little bit since it has to generate the SDF data!
	
	while( true ) { // find the next random font with a letter 'a' in it
		mFont = gl::SdfText::Font( gl::SdfText::Font::getNames()[Rand::randInt() % gl::SdfText::Font::getNames().size()], mFont.getSize() );
		if( mFont.getGlyphChar( 'a' ) == 0 )
			continue;
		console() << mFont.getName() << std::endl;
		mSdfText = gl::SdfText::create( mFont );
		break;
	}
}

void MeasureStringApp::update()
{
}

void MeasureStringApp::drawMeasuredStringRect( const std::string &str, const vec2 &baseline, const gl::SdfTextRef &sdfText, const gl::SdfText::DrawOptions &drawOptions )
{
	gl::ScopedColor color( Color( 1, 0, 0 ) );
	vec2 size = sdfText->measureString( str, drawOptions );

	Rectf r = Rectf( vec2( 0 ), size );
	r += baseline;
	r += vec2( 0, -size.y );

	gl::lineWidth( 2.0f );
	gl::drawStrokedRect( r );
}

void MeasureStringApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );

	auto drawOptions = gl::SdfText::DrawOptions()
		.premultiply( mPremultiply )
		.alignment( mAlignment )
		.justify( mJustify );
	
	std::string str;
	vec2 baseline;

	/*
	std::string str( "Granted, then, that certain transformations do happen, it is essential that we should regard them in the philosophic manner of fairy tales, not in the unphilosophic manner of science and the \"Laws of Nature.\" When we are asked why eggs turn into birds or fruits fall in autumn, we must answer exactly as the fairy godmother would answer if Cinderella asked her why mice turned into horses or her clothes fell from her at twelve o'clock. We must answer that it is MAGIC. It is not a \"law,\" for we do not understand its general formula." );
	Rectf boundsRect( 40, mSdfText->getAscent() + 40, getWindowWidth() - 40, getWindowHeight() - 40 );

	gl::color( ColorA( 1, 0.5f, 0.25f, 1.0f ) );

	auto drawOptions = gl::SdfText::DrawOptions()
		.premultiply( mPremultiply )
		.alignment( mAlignment )
		.justify( mJustify );

	mSdfText->drawStringWrapped( str, boundsRect, vec2( 0 ), drawOptions );
	*/

	// Draw alignment
	gl::color( Color::white() );
	std::string alignment = "LEFT";
	if( gl::SdfText::Alignment::RIGHT == mAlignment ) {
		alignment = "RIGHT";
	}
	else if( gl::SdfText::Alignment::CENTER == mAlignment ) {
		alignment = "CENTER";
	}
	str = alignment + ( mJustify ? " | JUSTIFIED" : "" );
	baseline = vec2( 10, 30 );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );

	// Draw elapsed seconds
	str = "Seconds that have gone by: " + toString( static_cast<float>( getElapsedSeconds() ) );
	baseline = vec2( 30, 100 );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );

	// Draw elapsed frames
	str = "Frames that have gone by: " + toString( getElapsedFrames() );
	baseline = vec2( 30, 160 );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );

	// Draw FPS
	str = toString( floor( getAverageFps() ) ) + " FPS" + std::string( mPremultiply ? " | premult" : "" );
	baseline = vec2( 10, getWindowHeight() - mSdfText->getDescent() );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );
    
    // Draw Font Name
	float fontNameWidth = mSdfText->measureString( mSdfText->getName() ).x;
	str = mSdfText->getName();
	baseline = vec2( getWindowWidth() - fontNameWidth - 10, getWindowHeight() - mSdfText->getDescent() );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );
}

void prepareSettings( App::Settings *settings )
{
}

CINDER_APP( MeasureStringApp, RendererGl, prepareSettings );

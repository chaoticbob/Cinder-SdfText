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
	void drawMeasuredStringRect( const std::string &str, const Rectf &fitRect, const gl::SdfTextRef &sdfText, const gl::SdfText::DrawOptions &drawOptions );
};

void MeasureStringApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../MeasureString/assets" );
#endif

	mFont = gl::SdfText::Font( loadAsset( "fonts/VarelaRound-Regular.ttf" ), 28 );
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
	Rectf r = sdfText->measureStringBounds( str, drawOptions );
	r += baseline;

	gl::lineWidth( 1.0f );
	gl::drawStrokedRect( r );
}

void MeasureStringApp::drawMeasuredStringRect( const std::string &str, const Rectf &fitRect, const gl::SdfTextRef &sdfText, const gl::SdfText::DrawOptions &drawOptions )
{
	gl::ScopedColor color( Color( 1, 0, 0 ) );
	Rectf r = sdfText->measureStringBoundsWrapped( str, fitRect, drawOptions ); 
	r += vec2( fitRect.x1, fitRect.y1 );

	gl::lineWidth( 1.0f );
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
	
	gl::color( Color::white() );

	std::string str;
	vec2 baseline;

	// Draw alignment
	str = "Font size: " + toString( mSdfText->getFont().getSize() );
	baseline = vec2( 10, 50 );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );

	// Draw elapsed seconds
	str = "Floating: " + toString( static_cast<float>( getElapsedSeconds() ) );
	baseline = vec2( 30, 100 );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );

	// Draw elapsed frames
	str = "Integer: " + toString( getElapsedFrames() );
	baseline = vec2( 30, 150 );
	mSdfText->drawString( str, baseline, drawOptions );
	drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );

	std::vector<std::string> items = {
		"Max Wondering Gnomes",
		"Puppies are the best!",
	};

	baseline = vec2( 30, 200 );
	for( const auto& item : items ) {
		str = item;
		mSdfText->drawString( str, baseline, drawOptions );
		drawMeasuredStringRect( str, baseline, mSdfText, drawOptions );
		baseline += vec2( 0, 50 );
	}

	// Draw lorem ipsum
	str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer lacinia nunc in nunc fermentum, in aliquam orci vulputate. In cursus metus nec lorem viverra tempor vel ac enim.";
	Rectf r = Rectf( 30, 300, 580, 400 );
	mSdfText->drawStringWrapped( str, r, vec2( 0 ), drawOptions.leading( -3 ) );
	drawMeasuredStringRect( str, r, mSdfText, drawOptions );

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
	//settings->setWindowSize( 1280, 720 );
}

CINDER_APP( MeasureStringApp, RendererGl, prepareSettings );

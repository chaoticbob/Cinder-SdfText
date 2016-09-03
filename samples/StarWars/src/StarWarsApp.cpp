#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const std::string sText =
"BRIEF TEXT ABOUT CINDER\n"
"\n"
"Cinder is a C++ library for programming with aesthetic intent - the sort of "
"development often called creative coding. This includes domains like graphics, "
"audio, video, and computational geometry. Cinder is cross-platform, with "
"official support for OS X, Windows, iOS, and WinRT.\n"
"\n"
"Cinder is production-proven, powerful enough to be the primary tool for "
"professionals, but still suitable for learning and experimentation.\n"
"\n"
"Cinder is released under the 2-Clause BSD License.\n"
"\n"
"Contributing... Cinder is developed through Github, and discussion is conducted "
"primarily via its forums. Code contributions, issue reports, and support requests "
"are welcome through these two avenues.\n"
"\n"
"Authors... Cinder's original author and current lead architect is Andrew Bell. "
"Significant portions of Cinder were derived from code coauthored with Hai Nguyen, "
"who continues to help steward the project along with Rich Eakin, Paul Houx, and a "
"growing, global community of users.\n"
"\n"
"http://libcinder.org";

//! \class StarWarsApp
//!
//!
class StarWarsApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::SdfText::Font		mFont;
	gl::SdfTextRef			mSdfText;
	CameraPersp				mCam;
	float					mTime = 0;
};

void StarWarsApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../StarWars/assets" );
#endif

	mCam.lookAt( vec3( 0, 0, 5 ), vec3( 0 ) );

	mFont = gl::SdfText::Font( loadAsset( "LibreFranklin-ExtraBold.ttf" ), 42 );
	mSdfText = gl::SdfText::create( getAssetPath( "" ) / "cached_font.sdft", mFont );
}

void StarWarsApp::keyDown( KeyEvent event )
{
}

void StarWarsApp::mouseDown( MouseEvent event )
{
}

void StarWarsApp::update()
{
	static float prevTime = static_cast<float>( getElapsedSeconds() );
	float curTime = static_cast<float>( getElapsedSeconds() );
	float dt = curTime - prevTime;
	mTime += dt;
	prevTime = curTime;
}

void StarWarsApp::draw()
{
	gl::setMatrices( mCam );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );

	// Tilt the text
	gl::rotate( toRadians( -50.0f ), vec3( 1, 0, 0 ) );

	// Scroll it
	gl::translate( vec2( 0, fmod( 0.25f * mTime, 17.0f ) - 2.0f ) );

	// Flip the y since coord sys is 3D
	gl::scale( vec2( 1, -1 ) );

	// Scale and fit the bounding rect to fit a width of 3.0
	gl::translate( vec2( -1.5, 0 ) );
	gl::scale( vec3( 3.0f / 640.0f ) );

	// Set color
	gl::color( Color8u( 255, 213, 19 ) );

	// Draw it!
	auto drawOptions = gl::SdfText::DrawOptions()
		.premultiply()
		.alignment( gl::SdfText::Alignment::LEFT )
		.justify();
	mSdfText->drawStringWrapped( sText, Rectf( 0, 0, 640, 480 ), vec2( 0 ), drawOptions );
}

void prepareSettings( App::Settings *settings )
{
	settings->setWindowSize( 1280, 720 );
}

CINDER_APP( StarWarsApp, RendererGl, prepareSettings );

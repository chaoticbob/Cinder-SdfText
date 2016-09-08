#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//! \class BasciApp
//!
//!
class SaveLoadApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::SdfTextRef		mStatusText;
	gl::SdfText::Font	mFont;
	gl::SdfTextRef		mSdfText;
	bool				mPremultiply = false;
	bool				mUseSdftFile = true;

	std::vector<std::string>			mSdftFileNames;
	std::map<std::string, std::string>	mFontFiles;
	size_t								mSdftIndex = 0;
	std::string							mCurrentSdfFile;

};

void SaveLoadApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../SaveLoad/assets" );
#endif

	mSdftFileNames = {
		"AlfaSlabOne.sdft",
		"Candal.sdft",
		"Cinzel.sdft",
		"FontdinerSwanky.sdft",
		"Lobster.sdft",
		"LuckiestGuy.sdft",
		"Orbitron.sdft",
		"Righteous.sdft",
		"Syncopate.sdft",
		"VarelaRound.sdft"
	};



	mFontFiles["AlfaSlabOne.sdft"]		= "fonts/AlfaSlabOne-Regular.ttf";
	mFontFiles["Audiowide.sdft"]		= "fonts/Audiowide-Regular.ttf";
	mFontFiles["Candal.sdft"]			= "fonts/Candal.ttf";
	mFontFiles["Cinzel.sdft"]			= "fonts/Cinzel-Regular.ttf";
	mFontFiles["FontdinerSwanky.sdft"]	= "fonts/FontdinerSwanky.ttf";
	mFontFiles["Lobster.sdft"]			= "fonts/Lobster-Regular.ttf";
	mFontFiles["LuckiestGuy.sdft"]		= "fonts/LuckiestGuy.ttf";
	mFontFiles["Orbitron.sdft"]			= "fonts/Orbitron-Regular.ttf";
	mFontFiles["Righteous.sdft"]		= "fonts/Righteous-Regular.ttf";
	mFontFiles["Syncopate.sdft"]		= "fonts/Syncopate-Regular.ttf";
	mFontFiles["VarelaRound.sdft"]		= "fonts/VarelaRound-Regular.ttf";

	mCurrentSdfFile = mSdftFileNames[mSdftIndex];

	mStatusText = gl::SdfText::create( getAssetPath( "" ) / "VarelaRound.sdft", gl::SdfText::Font( loadAsset( mFontFiles["VarelaRound.sdft"] ), 24.0f ) );
	mSdfText = gl::SdfText::create( getAssetPath( "" ) / mCurrentSdfFile, gl::SdfText::Font( loadAsset( mFontFiles[mCurrentSdfFile] ), 24.0f ) );
	mFont = mSdfText->getFont();
}

void SaveLoadApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '=':
		case '+':
			if( mUseSdftFile ) {
				mSdfText = gl::SdfText::load( getAssetPath( mCurrentSdfFile ), mFont.getSize() + 1 );
				mFont = mSdfText->getFont();
			}
			else {
				mFont = gl::SdfText::Font( mFont.getName(), mFont.getSize() + 1 );
				mSdfText = gl::SdfText::create( mFont );
			}
		break;
		case '-':
			if( mUseSdftFile ) {
				mSdfText = gl::SdfText::load( getAssetPath( mCurrentSdfFile ), mFont.getSize() - 1 );
				mFont = mSdfText->getFont();
			}
			else {
				mFont = gl::SdfText::Font( mFont.getName(), mFont.getSize() - 1 );
				mSdfText = gl::SdfText::create( mFont );
			}
		break;
		case 'p':
		case 'P':
			mPremultiply = ! mPremultiply;
		break;
		case 's':
		case 'S': {
			if( ! mUseSdftFile ) {
				// Save the font to a SDFT file
				console() << "Saving " << mSdfText->getName() << "..." << std::endl;
				gl::SdfText::save( getAssetPath( "" ) / "system_font.sdft", mSdfText );
				// Reload the font from the SDFT file
				mSdfText = gl::SdfText::load( getAssetPath( "" ) / "system_font.sdft" );
				mCurrentSdfFile = "system_font.sdft";
				mUseSdftFile = true;
			}
		}
		break;
		case 'n':
		case 'N': {
			++mSdftIndex;
			mSdftIndex %= mSdftFileNames.size();
			mCurrentSdfFile = mSdftFileNames[mSdftIndex];
			mSdfText = gl::SdfText::create( getAssetPath( "" ) / mCurrentSdfFile, gl::SdfText::Font( loadAsset( mFontFiles[mCurrentSdfFile] ), mFont.getSize() ) );
			mFont = mSdfText->getFont();
			mUseSdftFile = true;
		}
		break;
	}
}

void SaveLoadApp::mouseDown( MouseEvent event )
{
	// NOTE: This may take a little bit since it has to generate the SDF data!
	while( true ) { // find the next random font with a letter 'a' in it
		mFont = gl::SdfText::Font( gl::SdfText::Font::getNames()[Rand::randInt() % gl::SdfText::Font::getNames().size()], mFont.getSize() );
		if( mFont.getGlyphChar( 'a' ) == 0 )
			continue;
		console() << mFont.getName() << std::endl;
		mSdfText = gl::SdfText::create( mFont );
		mUseSdftFile = false;
		break;
	}
}

void SaveLoadApp::update()
{
}

void SaveLoadApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );
	
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

	std::string msg = "Rendering using system font";
	if( mUseSdftFile ) {
		msg = "Rendering with SDFT file: " + mCurrentSdfFile;
	}
	gl::color( Color( 1, 1, 1 ) );
	mStatusText->drawString( msg, vec2( 8, mStatusText->getFont().getHeight() - 6 ) );
}

void prepareSettings( App::Settings *settings )
{
}

CINDER_APP( SaveLoadApp, RendererGl, prepareSettings );

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/gl/SdfTextMesh.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

//! \class BasicMeshApp
//!
//!
class BasicMeshApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::SdfText::Font		mFont;
	gl::SdfTextRef			mSdfText;
	gl::SdfTextMeshRef		mSdfTextMesh;
	bool					mPremultiply = false;
	gl::SdfText::Alignment	mAlignment = gl::SdfText::Alignment::LEFT;
	bool					mJustify = false;

	gl::SdfTextMesh::RunRef	mBlockRun;
	gl::SdfTextMesh::RunRef mAlignmentRun;
	gl::SdfTextMesh::RunRef	mFpsRun;
	gl::SdfTextMesh::RunRef	mFontNameRun;

	void buildMesh();
};

void BasicMeshApp::buildMesh()
{
	// NOTE: Shows some different ways to add a gl::SdfTextMesh::run

	mSdfTextMesh = gl::SdfTextMesh::create();

	std::string blockText( "Granted, then, that certain transformations do happen, it is essential that we should regard them in the philosophic manner of fairy tales, not in the unphilosophic manner of science and the \"Laws of Nature.\" When we are asked why eggs turn into birds or fruits fall in autumn, we must answer exactly as the fairy godmother would answer if Cinderella asked her why mice turned into horses or her clothes fell from her at twelve o'clock. We must answer that it is MAGIC. It is not a \"law,\" for we do not understand its general formula." );
	Rectf blockBoundsRect( 40, mSdfText->getAscent() + 40, getWindowWidth() - 40, getWindowHeight() - 40 );
	gl::SdfTextMesh::Run::Options blockOptions = gl::SdfTextMesh::Run::Options().setAlignment( mAlignment ).setJustify( mJustify );
	mBlockRun = mSdfTextMesh->appendTextWrapped( blockText, mSdfText, blockBoundsRect, blockOptions );

	vec2 baseline = vec2( 10, 30 );
	mAlignmentRun = gl::SdfTextMesh::Run::create( "LEFT", mSdfText, baseline );
	mSdfTextMesh->appendText( mAlignmentRun );

	baseline = vec2( 10, getWindowHeight() - mSdfText->getDescent() );
	mFpsRun = gl::SdfTextMesh::Run::create( "fps", mSdfText, baseline );
	mSdfTextMesh->appendText( mFpsRun );

	float fontNameWidth = mSdfText->measureString( mSdfText->getName() ).x;
	baseline = vec2( getWindowWidth() - fontNameWidth - 10, getWindowHeight() - mSdfText->getDescent() );
	mSdfTextMesh->appendText( mFont.getName(), mSdfText, baseline );
}

void BasicMeshApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../BasicMesh/assets" );
#endif

#if defined( CINDER_COCOA_TOUCH )
	mFont = gl::SdfText::Font( "Cochin-Italic", 24 );
#elif defined( CINDER_COCOA )
	mFont = gl::SdfText::Font( "Helvetica", 24 );
#else
	mFont = gl::SdfText::Font( "Arial", 24 );
#endif
	mSdfText = gl::SdfText::create( getAssetPath( "" ) / "cached_font.sdft", mFont );
	buildMesh();
}

void BasicMeshApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '=':
		case '+':
			mFont = gl::SdfText::Font( mFont.getName(), mFont.getSize() + 1 );
			mSdfText = gl::SdfText::create( mFont );
		break;
		case '-':
			mFont = gl::SdfText::Font( mFont.getName(), mFont.getSize() - 1 );
			mSdfText = gl::SdfText::create( mFont );
		break;
		case 'p':
		case 'P':
			mPremultiply = ! mPremultiply;
		break;
		case 'l':
		case 'L':
			mAlignment = gl::SdfText::Alignment::LEFT;
			mBlockRun->setAlignment( mAlignment );
		break;
		case 'r':
		case 'R':
			mAlignment = gl::SdfText::Alignment::RIGHT;
			mBlockRun->setAlignment( mAlignment );
		break;
		case 'c':
		case 'C':
			mAlignment = gl::SdfText::Alignment::CENTER;
			mBlockRun->setAlignment( mAlignment );
		break;
		case 'j':
		case 'J':
			mJustify = ! mJustify;
			mBlockRun->setJustify( mJustify );
		break;
	}
}

void BasicMeshApp::mouseDown( MouseEvent event )
{
	// NOTE: This may take a little bit since it has to generate the SDF data!
	
	while( true ) { // find the next random font with a letter 'a' in it
		mFont = gl::SdfText::Font( gl::SdfText::Font::getNames()[Rand::randInt() % gl::SdfText::Font::getNames().size()], mFont.getSize() );
		if( mFont.getGlyphChar( 'a' ) == 0 )
			continue;
		console() << mFont.getName() << std::endl;
		mSdfText = gl::SdfText::create( mFont );
		buildMesh();
		break;
	}
}

void BasicMeshApp::update()
{
	std::string str = toString( floor( getAverageFps() ) ) + " FPS" + std::string( mPremultiply ? " | premult" : "" );
	mFpsRun->setText( str );

	str = "LEFT";
	if( gl::SdfText::Alignment::RIGHT == mAlignment ) {
		str = "RIGHT";
	}
	else if( gl::SdfText::Alignment::CENTER == mAlignment ) {
		str = "CENTER";
	}
	mAlignmentRun->setText( str + ( mJustify ? " | JUSTIFIED" : "" ) );
}

void BasicMeshApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::color( ColorA( 1, 0.5f, 0.25f, 1.0f ) );
	mSdfTextMesh->draw( mPremultiply );
}

void prepareSettings( App::Settings *settings )
{
}

CINDER_APP( BasicMeshApp, RendererGl, prepareSettings );

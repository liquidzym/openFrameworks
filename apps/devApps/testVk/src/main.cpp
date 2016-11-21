#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	
	ofSetLoggerChannel( std::make_shared<ofConsoleLoggerChannel>() );

	ofVkWindowSettings settings;
	settings.setVkVersion(1, 0, 21 );
	settings.numSwapchainImages = 2;
	settings.numVirtualFrames = 3;
	settings.swapchainType = ::vk::PresentModeKHR::eMailbox;

#ifdef NDEBUG
	settings.useDebugLayers = false;
#else
	settings.useDebugLayers = true;
#endif

	ofCreateWindow( settings );

	ofRunApp(new ofApp());
}

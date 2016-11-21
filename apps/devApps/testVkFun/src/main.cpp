#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main(){

	auto consoleLogger = new ofConsoleLoggerChannel();
	ofSetLoggerChannel( std::shared_ptr<ofBaseLoggerChannel>( consoleLogger, []( ofBaseLoggerChannel * lhs){} ) );

	ofVkWindowSettings settings;
	settings.setVkVersion( 1, 0, 26 );
	settings.numSwapchainImages = 3;
	settings.numVirtualFrames = 3;
	settings.swapchainType = ::vk::PresentModeKHR::eMailbox;

#ifdef NDEBUG
	settings.useDebugLayers = false;
#else
	settings.useDebugLayers = true;
#endif

	ofCreateWindow( settings );

	ofRunApp( new ofApp() );

}

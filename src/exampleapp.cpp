// Local Includes
#include "exampleapp.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "planeloggercomponent.h"

// External Includes
#include <utility/fileutils.h>
#include <nap/logger.h>
#include <inputrouter.h>
#include <rendergnomoncomponent.h>
#include <perspcameracomponent.h>
#include <nap/datetime.h>
#include <flightstate.h>

namespace nap 
{    
    bool CoreApp::init(utility::ErrorState& error)
    {
		// Retrieve services
		mSceneService	= getCore().getService<nap::SceneService>();

		// Fetch the resource manager
        mResourceManager = getCore().getResourceManager();

		// Get the scene that contains our entities and components
		mScene = mResourceManager->findObject<Scene>("Scene");
		if (!error.check(mScene != nullptr, "unable to find scene with name: %s", "Scene"))
			return false;

		// Get the camera and origin Gnomon entity
        mPlaneLoggerEntity = mScene->findEntity("PlaneLoggerEntity");

        // Cap the framerate
        setFramerate(30.0f);
        capFramerate(true);

		// All done!
        return true;
    }


    int CoreApp::shutdown()
    {
		return 0;
    }


	// Update app
    void CoreApp::update(double deltaTime)
    {
    }
}

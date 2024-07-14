#pragma once

// Core includes
#include <nap/resourcemanager.h>
#include <nap/resourceptr.h>

// Module includes
#include <renderservice.h>
#include <imguiservice.h>
#include <sceneservice.h>
#include <inputservice.h>
#include <scene.h>
#include <entity.h>
#include <app.h>

#include "restclient.h"
#include "database.h"

namespace nap 
{
	using namespace rtti;

    /**
     * Example application, called from within the main loop.
	 * 
	 * Use this app as a template for other apps that are created directly in 'source'.
	 * This example links to and uses it's own custom module: 'napexample'.
	 * More information and documentation can be found at: https://www.napframework.com/doxygen/
     */
    class CoreApp : public App 
	{
    public:
		/**
		 * Constructor
		 */
        CoreApp(nap::Core& core) : App(core) {}

        /**
         * Initialize all the services and app specific data structures
		 * @param error contains the error code when initialization fails
		 * @return if initialization succeeded
         */
        bool init(utility::ErrorState& error) override;

		/**
		 * Update is called every frame, before render.
		 * @param deltaTime the time in seconds between calls
		 */
		void update(double deltaTime) override;

        /**
		 * Called when the app is shutting down after quit() has been invoked
		 * @return the application exit code, this is returned when the main loop is exited
         */
        int shutdown() override;
    private:
        ResourceManager*			mResourceManager = nullptr;		///< Manages all the loaded data
		SceneService*				mSceneService = nullptr;		///< Manages all the objects in the scene
		ObjectPtr<Scene>			mScene = nullptr;				///< Pointer to the main scene
        ObjectPtr<EntityInstance>	mPlaneLoggerEntity = nullptr;
	};
}

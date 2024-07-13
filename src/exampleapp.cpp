// Local Includes
#include "exampleapp.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

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
		mRenderService	= getCore().getService<nap::RenderService>();
		mSceneService	= getCore().getService<nap::SceneService>();
		mInputService	= getCore().getService<nap::InputService>();
		mGuiService		= getCore().getService<nap::IMGuiService>();

		// Fetch the resource manager
        mResourceManager = getCore().getResourceManager();

		// Get the render window
		mRenderWindow = mResourceManager->findObject<nap::RenderWindow>("Window");
		if (!error.check(mRenderWindow != nullptr, "unable to find render window with name: %s", "Window"))
			return false;

		// Get the scene that contains our entities and components
		mScene = mResourceManager->findObject<Scene>("Scene");
		if (!error.check(mScene != nullptr, "unable to find scene with name: %s", "Scene"))
			return false;

		// Get the camera and origin Gnomon entity
		mCameraEntity = mScene->findEntity("CameraEntity");
		mGnomonEntity = mScene->findEntity("GnomonEntity");

        //
        mRestClient = mResourceManager->findObject<RestClient>("RestClient");

        std::vector<std::unique_ptr<APIBaseValue>> params;
        params.emplace_back(std::make_unique<APIValue<int>>("faa", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("satellite", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("mlat", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("flarm", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("adsb", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("gnd", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("air", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("vehicles", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("estimated", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("maxage", 14400));
        params.emplace_back(std::make_unique<APIValue<int>>("gliders", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("stats", 1));
        params.emplace_back(std::make_unique<APIValue<int>>("limit", 5000));
        params.emplace_back(std::make_unique<APIDoubleArray>("bounds", std::vector<double>{53.445884435606054, 50.749405057563486, 3.5163031843031223, 7.9136148705580505}));

        setFramerate(30.0f);
        capFramerate(true);

		// All done!
        return true;
    }


    // Render app
    void CoreApp::render()
    {
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording the render commands for the main render window
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Get Perspective camera to render with
			auto& perp_cam = mCameraEntity->getComponent<PerspCameraComponentInstance>();

			// Add Gnomon
			std::vector<nap::RenderableComponentInstance*> components_to_render
			{
				&mGnomonEntity->getComponent<RenderGnomonComponentInstance>()
			};

			// Render Gnomon
			mRenderService->renderObjects(*mRenderWindow, perp_cam, components_to_render);

			// Draw GUI elements
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Proceed to next frame
		mRenderService->endFrame();
    }


    void CoreApp::windowMessageReceived(WindowEventPtr windowEvent)
    {
		mRenderService->addEvent(std::move(windowEvent));
    }


    void CoreApp::inputMessageReceived(InputEventPtr inputEvent)
    {
		// If we pressed escape, quit the loop
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
				quit();

			if (press_event->mKey == nap::EKeyCode::KEY_f)
				mRenderWindow->toggleFullscreen();
		}
		mInputService->addEvent(std::move(inputEvent));
    }


    int CoreApp::shutdown()
    {
		return 0;
    }


	// Update app
    void CoreApp::update(double deltaTime)
    {
		// Use a default input router to forward input events (recursively) to all input components in the scene
		// This is explicit because we don't know what entity should handle the events from a specific window.
		nap::DefaultInputRouter input_router(true);
		mInputService->processWindowEvents(*mRenderWindow, input_router, { &mScene->getRootEntity() });

        ImGui::Begin("Controls");
        if(ImGui::Button("Request"))
        {
            std::vector<std::unique_ptr<APIBaseValue>> params;
            params.emplace_back(std::make_unique<APIValue<int>>("faa", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("satellite", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("mlat", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("flarm", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("adsb", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("gnd", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("air", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("vehicles", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("estimated", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("maxage", 14400));
            params.emplace_back(std::make_unique<APIValue<int>>("gliders", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("stats", 1));
            params.emplace_back(std::make_unique<APIValue<int>>("limit", 5000));
            params.emplace_back(std::make_unique<APIDoubleArray>("bounds", std::vector<double>{53.445884435606054, 50.749405057563486, 3.5163031843031223, 7.9136148705580505}));
            mRestClient->get("/zones/fcgi/feed.js", params, [](const RestResponse& response)
            {
                /*
                 * string s = "ABCDEF";
                    Document d(kObjectType);
                    Value data(kObjectType);
                    Value value;
                    value.SetString(s.c_str(), d.GetAllocator());
                    data.AddMember("value", value, d.GetAllocator());
                    d.AddMember("data", data, d.GetAllocator());

                    std::cout << jsonToString(d);
                 */

                rapidjson::Document d(rapidjson::kObjectType);
                d.Parse(response.mData.c_str());
                for (rapidjson::Value::ConstMemberIterator p = d.MemberBegin(); p != d.MemberEnd(); ++p)
                {
                    if(p->value.IsArray())
                    {
                        for (rapidjson::Value::ConstValueIterator q = p->value.Begin(); q != p->value.End(); ++q)
                        {
                            if(q->IsInt())
                            {
                                //q->Get
                                nap::Logger::info("int: %d", q->GetInt());
                            }
                            else if(q->IsString())
                            {
                                nap::Logger::info("string: %s", q->GetString());
                            }else if(q->IsFloat())
                            {
                                nap::Logger::info("float: %f", q->GetFloat());
                            }
                        }

                    }
                }

            }, [](const utility::ErrorState& error)
            {
                nap::Logger::error("error! %s", error.toString().c_str());
            });

        }
        if(ImGui::Button("Add 100000 records"))
        {
            for(int i = 0; i < 100000; i++)
            {
                FlightStatesData state;
                state.mData = "test";
                auto now = getCurrentDateTime();
                std::string now_format = utility::stringFormat("%d%02d%02d%02d%02d%02d", now.getYear(), now.getMonth(), now.getDay(), now.getHour(), now.getMinute(), now.getSecond());
                uint64 now_uint64 = std::stoull(now_format);
                state.mTimeStamp = now_uint64 + i;

                utility::ErrorState err;
                if(!mTableDatabase->add(state, err))
                {
                    nap::Logger::error("error! %s", err.toString().c_str());
                }
            }
        }
        if(ImGui::Button("Delete all records"))
        {
            utility::ErrorState err;
            if(!mTableDatabase->clear(err))
            {
                nap::Logger::error("error! %s", err.toString().c_str());
            }
        }
        ImGui::End();
    }
}

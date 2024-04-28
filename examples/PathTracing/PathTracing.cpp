#include "KBS.h"
#include "Core/Entry.h"
#include "Renderer/PTRenderer.h"
#include <iostream>

using namespace kbs;


class PathTracingApplication : public kbs::Application
{
public:

	PathTracingApplication(kbs::ApplicationCommandLine& commandLine)
		: Application(commandLine){}

	virtual void OnUpdate() override;
	
	virtual void BeforeRun() override;
private:
	Entity					modelEntity;
	kbs::vec3				rotateAxis = kbs::math::normalize(kbs::vec3(1, 1, 1));
	Angle					angle = 0;
};


namespace kbs
{
	Application* CreateApplication(ApplicationCommandLine& commandLines)
	{
		auto app = new PathTracingApplication(commandLines);
		return app;
	}
}

ptr<Scene> scene;
ptr<PTRenderer>     renderer;
Entity					  mainCamera;




void PathTracingApplication::BeforeRun()
{
	renderer = std::make_shared<PTRenderer>();
	scene = std::make_shared<Scene>();

	//Singleton::GetInstance<FileSystem<ShaderManager>>()->AddSearchPath(_ROOT_DIRECTORY);
	Singleton::GetInstance<FileSystem<ModelManager>>()->AddSearchPath(ASSET_ROOT_DIRECTORY);
	auto assetManager = Singleton::GetInstance<AssetManager>();
	
	RendererCreateInfo info;
	info.instance.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	info.instance.AddLayer(GVK_LAYER_DEBUG);
	info.instance.AddLayer(GVK_LAYER_FPS_MONITOR);
	info.instance.AddInstanceExtension(GVK_INSTANCE_EXTENSION_SHADER_PRINT);
	
	info.device.AddDeviceExtension(GVK_DEVICE_EXTENSION_DEBUG_MARKER);
	info.device.AddDeviceExtension(GVK_DEVICE_EXTENSION_RAYTRACING);
	info.device.AddDeviceExtension(GVK_DEVICE_EXTENSION_BINDLESS_IMAGE);
	info.device.AddDeviceExtension(GVK_DEVICE_EXTENSION_INT64);

	renderer->Initialize(m_Window, info);
	
	RenderAPI api = renderer->GetAPI();
	Singleton::GetInstance<AssetManager>()->GetTextureManager()->LoadDefaultTextures(api);
	scene = std::make_shared<Scene>();

	renderer->SetMaxDepth(3);

	{
		ModelLoadOption loadOpt;
		loadOpt.flags = ModelLoadOption::RayTracingSupport | ModelLoadOption::SkipTransparent;

		ModelID modelID = assetManager->GetModelManager()->LoadFromGLTF("models/sponza/sponza.gltf", api, loadOpt).value();
		ptr<Model> model = assetManager->GetModelManager()->GetModel(modelID).value();

		TransformComponent modelTransform;
		modelTransform.parent = scene->GetRootID();
		modelTransform.position = kbs::vec3(0);
		modelTransform.rotation = kbs::quat();
		modelTransform.scale = kbs::vec3(1);

		
		ModelInstantiateOption instOpt;
		instOpt.flags = ModelInstantiateOption::RayTracingSupport;
		modelEntity = model->Instantiate(scene, "sponza", View<ptr<ModelMaterialSet>>(), modelTransform, instOpt);
	}
	
	// add main camera to scene
	{
		CameraComponent camera(1000, 0.1, 1, kbs::pi / 2);
		TransformComponent trans(kbs::vec3(0, 3 , 0), kbs::quat(1, 0, 0, 0), kbs::vec3(1, 1, 1));
		trans.parent = scene->GetRootID();

		mainCamera = scene->CreateMainCamera(trans, camera);
	}

	// add main light to scene
	{
		{
			LightComponent light;
			light.type = LightComponent::Sphere;
			light.intensity = kbs::vec3(50.0, 0.0, 0.0);
			light.sphere.radius = 0.1;

			TransformComponent trans = scene->CreateTransform({}, kbs::vec3(3.0, 1.0, 0), kbs::quat(1, 0, 0, 0), kbs::vec3(1, 1, 1));
			auto mainLight = scene->CreateEntity("mainLight");

			mainLight.AddComponent<LightComponent>(light);
			mainLight.AddComponent<TransformComponent>(trans);
		}
		
		
		{
			LightComponent env;
			env.type = LightComponent::ConstantEnvoriment;
			env.intensity = kbs::vec3(2.0, 2.0, 2.0);

			TransformComponent trans = scene->CreateTransform({}, kbs::vec3(0.0, 0.0, 0), kbs::quat(1, 0, 0, 0), kbs::vec3(1, 1, 1));
			auto light = scene->CreateEntity("light1");
			light.AddComponent<LightComponent>(env);
			light.AddComponent<TransformComponent>(trans);
		}
		
		/*
		{
			LightComponent env;
			env.type = LightComponent::Directional;
			env.intensity = kbs::vec3(10.0, 10.0, 10.0);

			TransformComponent trans = scene->CreateTransform({}, kbs::vec3(0.0, 0.0, 0), kbs::math::axisAngle(kbs::vec3(1,0,0), kbs::Angle::FromDegree(87)), kbs::vec3(1, 1, 1));
			auto light = scene->CreateEntity("light2");
			light.AddComponent<LightComponent>(env);
			light.AddComponent<TransformComponent>(trans);
		}
		*/
	}
}


float theta = 90.0f;
float phi = 90.0f;

void PathTracingApplication::OnUpdate()
{
	
	{
		// angle = Angle::FromDegree(100.f * m_Timer->TotalTime());
		TransformComponent& comp = mainCamera.GetComponent<TransformComponent>();
		Transform cameraTrans(mainCamera.GetComponent<TransformComponent>(), mainCamera);
		cameraTrans.SetLocalRotation(kbs::math::axisAngle(kbs::vec3(0, 1, 0), angle));

		float sinTheta = math::sin(Angle::FromDegree(theta));
		float cosTheta = math::cos(Angle::FromDegree(theta));
		float sinPhi = math::sin(Angle::FromDegree(phi));
		float cosPhi = math::cos(Angle::FromDegree(phi));

		kbs::vec3 dir = kbs::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
		// cameraTrans.FaceDirection(dir);

		EventManager* eventManager = Singleton::GetInstance<EventManager>();

		if (m_Window->GetGvkWindow()->MouseMove() && m_Window->GetGvkWindow()->KeyHold(GVK_MOUSE_1))
		{
			auto dmouse = m_Window->GetGvkWindow()->GetMouseOffset();
			phi += dmouse.x * 0.3;
			theta += dmouse.y * 0.3;

			if (phi > 360.0f)
			{
				phi -= 360.0f;
			}
			else if (phi < 0.0f)
			{
				phi += 360.0f;
			}

			theta = glm::clamp(theta, 1e-4f, 180.0f - 1e-4f);

			//eventManager->BoardcastEvent(PTAccumulationUpdateEvent());
		}

		if (m_Window->GetGvkWindow()->KeyHold(GVK_KEY_D))
		{
			angle.radius += m_Timer->DeltaTime();
			eventManager->BoardcastEvent(PTAccumulationUpdateEvent());
		}

		else if(m_Window->GetGvkWindow()->KeyHold(GVK_KEY_A))
		{
			angle.radius -= m_Timer->DeltaTime();
			eventManager->BoardcastEvent(PTAccumulationUpdateEvent());
		}

		// cameraTrans.SetLocalPosition(kbs::vec3(0, d + 3, 0));
		comp = cameraTrans.GetComponent();
	}
	
	 renderer->RenderScene(scene);
}


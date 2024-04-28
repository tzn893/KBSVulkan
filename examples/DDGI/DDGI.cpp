#include "KBS.h"
#include "Core/Entry.h"
#include <iostream>

using namespace kbs;


class DeferredApplication : public kbs::Application
{
public:

	DeferredApplication(kbs::ApplicationCommandLine& commandLine)
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
		auto app = new DeferredApplication(commandLines);
		return app;
	}
}

ptr<Scene> scene;
ptr<DeferredRenderer>	renderer;

float theta = 90.0f;
float phi = 90.0f;



void DeferredApplication::BeforeRun()
{
	scene = std::make_shared<Scene>();
	renderer = std::make_shared<DeferredRenderer>();


	Singleton::GetInstance<FileSystem<ShaderManager>>()->AddSearchPath(COMPLEX_SCENE_ROOT_DIRECTORY);
	Singleton::GetInstance<FileSystem<ModelManager>>()->AddSearchPath(ASSET_ROOT_DIRECTORY);
	auto assetManager = Singleton::GetInstance<AssetManager>();
	
	RendererCreateInfo info;
	info.instance.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	info.instance.AddInstanceExtension(GVK_INSTANCE_EXTENSION_SHADER_PRINT);
	info.instance.AddLayer(GVK_LAYER_DEBUG);
	info.instance.AddLayer(GVK_LAYER_FPS_MONITOR);
	info.device.AddDeviceExtension(GVK_DEVICE_EXTENSION_DEBUG_MARKER);

	renderer->Initialize(m_Window, info);
	
	RenderAPI api = renderer->GetAPI();
	Singleton::GetInstance<AssetManager>()->GetTextureManager()->LoadDefaultTextures(api);
	scene = std::make_shared<Scene>();

	{
		ModelLoadOption loadOpt;
		loadOpt.flags = 0;

		ModelID modelID = assetManager->GetModelManager()->LoadFromGLTF("models/sponza/sponza.gltf", api, loadOpt).value();
		ptr<Model> model = assetManager->GetModelManager()->GetModel(modelID).value();

		TransformComponent modelTransform;
		modelTransform.parent = scene->GetRootID();
		modelTransform.position = kbs::vec3(0);
		modelTransform.rotation = kbs::quat();
		modelTransform.scale = kbs::vec3(1);

		ShaderID mrtShaderID = renderer->GetDeferredPass()->GetMRTShader()->GetShaderID();
		ShaderID shadowShaderID = assetManager->GetShaderManager()->GetDepthOnlyShader()->GetShaderID();

		ptr<ModelMaterialSet> materialSet[] = { 
			model->CreateMaterialSetForModel(mrtShaderID, api).value(),
			model->CreateMaterialSetForModel(shadowShaderID, api).value() 
		};
		
		modelEntity = model->Instantiate(scene, "sponza", View(materialSet, 2), modelTransform, ModelInstantiateOption());
	}
	
	// add main camera to scene
	{
		CameraComponent camera(1000, 0.1, 1, kbs::pi / 2);
		TransformComponent trans = scene->CreateTransform({}, kbs::vec3(0,3,0), kbs::quat(), kbs::vec3(1, 1, 1));
	
		scene->CreateMainCamera(trans, camera);
	}

	// add lights to scene
	{
		{
			{
				LightComponent env;
				env.type = LightComponent::ConstantEnvoriment;
				env.intensity = kbs::vec3(1.0, 1.0, 1.0) * 0.5f;

				env.shadowCaster.castShadow = false;

				TransformComponent trans = scene->CreateTransform({}, kbs::vec3(0.0, 0.0, 0), kbs::quat(1, 0, 0, 0), kbs::vec3(1, 1, 1));
				auto light = scene->CreateEntity("light1");
				light.AddComponent<LightComponent>(env);
				light.AddComponent<TransformComponent>(trans);
			}

			
			{
				LightComponent env;
				env.type = LightComponent::Directional;
				env.intensity = kbs::vec3(5.0, 5.0, 5.0) * 1.4f;

				env.shadowCaster.castShadow = true;
				env.shadowCaster.windowWidth = 40.0f;
				env.shadowCaster.windowHeight = 40.0f;
				env.shadowCaster.distance = 100.0f;
				env.shadowCaster.bias = 1e-3f;

				TransformComponent trans = scene->CreateTransform({}, kbs::vec3(0.0, 0.0, 0), kbs::math::axisAngle(kbs::vec3(1,0,0), kbs::Angle::FromDegree(85)), kbs::vec3(1, 1, 1));
				auto light = scene->CreateEntity("light2");
				light.AddComponent<LightComponent>(env);
				light.AddComponent<TransformComponent>(trans);
			}
		}
	}
}

void DeferredApplication::OnUpdate()
{
	{
		auto mainCamera = scene->GetMainCamera();
		// angle = Angle::FromDegree(100.f * m_Timer->TotalTime());
		TransformComponent& comp = mainCamera.GetComponent<TransformComponent>();

		Transform cameraTrans(mainCamera.GetComponent<TransformComponent>(), mainCamera);
		cameraTrans.SetLocalRotation(kbs::math::axisAngle(kbs::vec3(0, 1, 0), angle));

		comp = cameraTrans.GetComponent();

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
		}

		else if (m_Window->GetGvkWindow()->KeyHold(GVK_KEY_A))
		{
			angle.radius -= m_Timer->DeltaTime();
		}

		renderer->RenderScene(scene);
	}
}

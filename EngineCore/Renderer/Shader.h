#pragma once
#include "Common.h"
#include "gvk.h"
#include "Renderer/ShaderParser.h"
#include "Scene/UUID.h"

namespace kbs
{
	// shader set usages
	// set 0 perObject : 0 object uniform buffer
	// set 1 perMaterial : 0 material uniform buffer(material variables), other buffers, other textures
	// set 2 perCamera : 0 camera uniform buffer, unused
	// set 3 perDraw : other buffers
	enum class ShaderSetUsage
	{
		perObject = 0,
		perMaterial = 1,
		perCamera = 2,
		perDraw = 3
	};

	class ShaderReflection
	{
	public:
		enum class VariableType
		{
			Int,
			Float,
			Float2,
			Float3,
			Float4,
			Mat4,
			Image2D
		};

		enum class BufferType
		{
			Uniform,
			Storage
		};

		enum class TextureType
		{
			CombinedImageSampler,
			StorageImage
		};

		struct VariableInfo
		{
			VariableType type;
			uint32_t     binding;
			uint32_t	 set;
			uint32_t	 offset;
			uint32_t	 stride;
		};

		struct BufferInfo
		{
			uint32_t binding;
			uint32_t set;
			BufferType type;
		};

		struct AccelerationStructureInfo
		{
			uint32_t binding;
			uint32_t set;
		};

		struct TextureInfo
		{
			TextureType type;
			uint32_t binding;
			uint32_t set;
			uint32_t arrayed;
			uint32_t depth;
			uint32_t dim;
		};

		bool			  GraphicsReflectionFromBindings(std::vector<SpvReflectDescriptorBinding*>& bindings, std::string& msg);
		bool			  ComputeReflectionFromBindings(std::vector<SpvReflectDescriptorBinding*>& bindings, std::string& msg);
		bool			  RayTracingReflectionFromBindings(std::vector<SpvReflectDescriptorBinding*>& bindings, std::string& msg);
		
		opt<VariableInfo> GetVariable(const std::string& name);
		uint32_t		  GetVariableBufferSize();

		opt<BufferInfo>	  GetBuffer(const std::string& name);
		opt<TextureInfo>  GetTexture(const std::string& name);
		opt<AccelerationStructureInfo> GetAS(const std::string& name);

		void			  IterateVariables(std::function<bool(const std::string&, VariableInfo&)>);
		void			  IterateTextures(std::function<bool(const std::string&, TextureInfo&)>);
		void			  IterateBuffers(std::function<bool(const std::string&, BufferInfo&)>);

		std::vector<uint32_t> GetOccupiedSetIndices();

	private:
		std::unordered_map<std::string, VariableInfo> m_VariableInfos;
		std::unordered_map<std::string, BufferInfo>   m_BufferInfos;
		std::unordered_map<std::string, TextureInfo>  m_TextureInfos;
		std::unordered_map<std::string, AccelerationStructureInfo> m_AccelStructInfos;
		uint32_t									  m_VariableBufferSize;
	};

	class ShaderManager;

	using ShaderID = UUID;

	class Shader
	{
	public:
		Shader(ShaderType type, std::string shaderPath, ShaderManager* manager, ShaderID id);
		virtual ~Shader() {}

		bool		IsGraphicsShader();
		bool		IsComputeShader();
		bool		IsRayTracingShader();

		ShaderType  GetShaderType();
		std::string GetShaderPath();
		ShaderID	GetShaderID();

		virtual void OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info) {}
		virtual void OnPipelineStateCreate(GvkComputePipelineCreateInfo& info) {}

		ShaderReflection& GetShaderReflection();
		
	protected:
		virtual bool GenerateReflection() { return false; }

		std::string			m_ShaderPath;
		ShaderType			m_ShaderType;
		ShaderReflection	m_Reflection;
		ShaderManager*      m_Manager;
		ShaderID			m_Id;
		friend class ShaderManager;
	};

	class GraphicsShader : public Shader
	{
	public:
		GraphicsShader(ShaderType type, std::string shaderPath, ShaderInfo& info, ShaderManager* manager, RenderPassFlags flag, ShaderID id, uint32_t blendLocationCount)
			:Shader(type, shaderPath, manager, id),raster(info.raster), blendState(info.blendState),depthStencilState(info.depthStencilState),m_RenderPassFlags(flag)
		{
			LoadShaderInfo(info, blendLocationCount);
		}

		RenderPassFlags GetRenderPassFlags();

	protected:
		void AssignRFDState(GvkGraphicsPipelineCreateInfo& info);
		void LoadShaderInfo(ShaderInfo& info, uint32_t blendLocationCount);

		GvkGraphicsPipelineCreateInfo::RasterizationStateCreateInfo raster;
		GvkGraphicsPipelineCreateInfo::FrameBufferBlendState        blendState;
		GvkGraphicsPipelineCreateInfo::DepthStencilStateInfo        depthStencilState;

		RenderPassFlags												m_RenderPassFlags;
	};

	class SurfaceShader : public GraphicsShader
	{
	public:
		SurfaceShader(ptr<gvk::Shader> fragmentShader, ShaderInfo& info, std::string shaderPath, ShaderManager* manager, RenderPassFlags flags, ShaderID id) :
			GraphicsShader(ShaderType::Surface, shaderPath, info, manager, flags, id, fragmentShader->GetOutputVariableCount()), frag(fragmentShader) {}

		void OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info) override;
	private:
		virtual bool GenerateReflection() override;

		ptr<gvk::Shader> frag;
	};

	class CustomVertexShader : public GraphicsShader
	{
	public:
		CustomVertexShader(ptr<gvk::Shader> vertexShader, ptr<gvk::Shader> fragmentShader, ShaderInfo& info, std::string shaderPath, ShaderManager* manager, RenderPassFlags flags, ShaderID id) :
			GraphicsShader(ShaderType::CustomVertex, shaderPath, info,manager, flags, id, fragmentShader->GetOutputVariableCount()),vert(vertexShader), frag(fragmentShader)
		{}

		void OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info) override;
	private:
		virtual bool GenerateReflection() override;

		ptr<gvk::Shader> vert;
		ptr<gvk::Shader> frag;
	};

	class MeshShader : public GraphicsShader
	{
	public:
		MeshShader(ptr<gvk::Shader> meshShader, ptr<gvk::Shader> taskShader, ptr<gvk::Shader> fragmentShader, ShaderInfo& info, std::string shaderPath, ShaderManager* manager, RenderPassFlags flags, ShaderID id) :
			GraphicsShader(ShaderType::MeshShader, shaderPath, info,manager, flags, id, fragmentShader->GetOutputVariableCount()), mesh(meshShader), task(taskShader), frag(fragmentShader)
		{}

		void OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info) override;
	private:
		virtual bool GenerateReflection() override;

		ptr<gvk::Shader> mesh;
		ptr<gvk::Shader> task;
		ptr<gvk::Shader> frag;
	};

	class ComputeShader : public Shader
	{
	public:
		ComputeShader(ptr<gvk::Shader> computeShader, std::string shaderPath, ShaderManager* manager, ShaderID id) :
			Shader(ShaderType::Compute, shaderPath, manager, id), comp(computeShader){}

		void OnPipelineStateCreate(GvkComputePipelineCreateInfo& info) override;

		ptr<gvk::Shader> GetGvkShader() { return comp; }

	private:
		virtual bool GenerateReflection() override;

		ptr<gvk::Shader> comp;
	};

	class RayTracingShader : public Shader
	{
	public:

		RayTracingShader(gvk::RayTracingPieplineCreateInfo pipelineCI, std::string shaderPath, ShaderManager* manager, ShaderID id):
			Shader(ShaderType::RayTracing, shaderPath, manager, id),  pipelineCI(pipelineCI){}
		
		gvk::RayTracingPieplineCreateInfo& OnPipelineStateCreate();

	private:
		virtual bool GenerateReflection() override;

		gvk::RayTracingPieplineCreateInfo pipelineCI;
	};


	class ShaderMacroSet
	{
		friend class ShaderManager;
	public:
		void Define(const char* def);
		void Define(const char* def, const char* value);
		void Remove(const char* def);
	private:
		uint32_t Find(const char* def);
		gvk::ShaderMacros GetShaderMacros();

		std::vector<std::string> values;
		std::vector<std::string> defs;
	};

	class ShaderManager
	{
	public:
		ShaderManager();

		void			 Initialize(ptr<gvk::Context> ctx);
		ShaderMacroSet&	 GetMacroSet();

		opt<ptr<Shader>> Load(const std::string& filePath);
		opt<ptr<Shader>> Reload(const std::string& filePath) { return Load(filePath); }

		opt<ptr<Shader>> Get(const ShaderID& id);
		opt<ptr<Shader>> GetByPath(const std::string& filePath);
		bool			 Exists(const std::string& filePath);
	private:
		opt<ShaderInfo>  LoadAndPreParse(const std::string& filePath, std::string& fileabsolutePath);
		opt<ptr<gvk::Shader>> CompileGvkShader(const std::string& preParsedShaderContent,const std::string& shaderFilePath, 
			const std::string& target,std::vector<const char*>& workingDirectories, std::string& msg);

		std::unordered_map<std::string, ShaderID> m_ShaderPathTable;
		std::unordered_map<ShaderID, ptr<Shader>> m_Shaders;
		ptr<gvk::Shader>		 m_StandardVertexShader;
		ptr<gvk::Context>		 m_Context;
		ShaderMacroSet			 m_Macros;

		friend class SurfaceShader;
	};
}

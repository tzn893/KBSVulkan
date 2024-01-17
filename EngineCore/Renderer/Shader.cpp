#include "Shader.h"
#include "Core/Log.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace kbs
{
	ShaderManager::ShaderManager()
	{
		
	}

	void ShaderManager::AddSearchingDirectory(const std::string& directory)
	{
		if (std::find(m_SearchingDirectories.begin(), m_SearchingDirectories.end() ,directory) == m_SearchingDirectories.end())
		{
			m_SearchingDirectories.push_back(directory);
		}
	}

	void ShaderManager::Initialize(ptr<gvk::Context> ctx)
	{
		const char* shaderDirectorys[] = { KBS_ROOT_DIRECTORY"/Renderer/Shader/" };
		auto standardVertex = ctx->CompileShader(
			"standard_vertex.vert", gvk::ShaderMacros(),
			shaderDirectorys, 1, shaderDirectorys, 1, nullptr);
		KBS_ASSERT(standardVertex.has_value(), "standard vertex shader must be compiled");
		m_StandardVertexShader = standardVertex.value();
		m_SearchingDirectories.push_back(KBS_ROOT_DIRECTORY"/Renderer/shader");

		m_Context = ctx;
	}


	opt<ptr<Shader>> kbs::ShaderManager::Load(const std::string& _filePath)
	{
		//if (!Exists(filePath))
		ShaderInfo  preParsedInfo;
		std::string absolutePath = _filePath;
		{
			opt<ShaderInfo> info = LoadAndPreParse(_filePath, absolutePath);
			if (!info.has_value())
			{
				return std::nullopt;
			}
			preParsedInfo = info.value();
		}
		std::string filePath = absolutePath;
		
			
		std::vector<const char*> workingDirectoryCStr;
		for (auto& s : m_SearchingDirectories) workingDirectoryCStr.push_back(s.c_str());
		
		std::string msg;
		ptr<Shader> loadedShader;

		ShaderID shaderID = UUID::GenerateUncollidedID(m_Shaders);

		switch (preParsedInfo.type)
		{
			case ShaderType::Surface:
				{
					auto frag = CompileGvkShader(preParsedInfo.fragmentShader, filePath, "frag", workingDirectoryCStr, msg);
					if (!frag.has_value())
					{
						KBS_WARN("fail to compile fragment shader in {} reason {}", filePath.c_str(), msg.c_str());
						return std::nullopt;
					}
					loadedShader = std::make_shared<SurfaceShader>(frag.value(), preParsedInfo, filePath, this, preParsedInfo.renderPassFlags, shaderID);
					break;
				}
			case ShaderType::CustomVertex:
				{
					auto vert = CompileGvkShader(preParsedInfo.vertexShader, filePath, "vert", workingDirectoryCStr, msg);
					if (!vert.has_value())
					{
						KBS_WARN("fail to compile vertex shader in {} reason {}", filePath.c_str(), msg.c_str());
						return std::nullopt;
					}
					opt<ptr<gvk::Shader>> frag;
					if (!preParsedInfo.fragmentShader.empty())
					{
						frag = CompileGvkShader(preParsedInfo.fragmentShader, filePath, "frag", workingDirectoryCStr, msg);
						if (!frag.has_value())
						{
							KBS_WARN("fail to compile fragment shader in {} reason {}", filePath.c_str(), msg.c_str());
							return std::nullopt;
						}
					}
					loadedShader = std::make_shared<CustomVertexShader>(vert.value(), frag.value() ? frag.value() : nullptr, preParsedInfo, filePath, this, preParsedInfo.renderPassFlags, shaderID);
					break;
				}
			case ShaderType::MeshShader:
				{
					auto mesh = CompileGvkShader(preParsedInfo.meshShader, filePath, "vert", workingDirectoryCStr, msg);
					if (!mesh.has_value())
					{
						KBS_WARN("fail to compile mesh shader in {} reason {}", filePath.c_str(), msg.c_str());
						return std::nullopt;
					}
					opt<ptr<gvk::Shader>> frag;
					if (!preParsedInfo.fragmentShader.empty())
					{
						frag = CompileGvkShader(preParsedInfo.fragmentShader, filePath, "frag", workingDirectoryCStr, msg);
						if (!frag.has_value())
						{
							KBS_WARN("fail to compile fragment shader in {} reason {}", filePath.c_str(), msg.c_str());
							return std::nullopt;
						}
					}
					opt<ptr<gvk::Shader>> task;
					if (!preParsedInfo.taskShader.empty())
					{
						task = CompileGvkShader(preParsedInfo.taskShader, filePath, "task", workingDirectoryCStr, msg);
						if (!task.has_value())
						{
							KBS_WARN("fail to compile task shader in {} reason {}", filePath.c_str(), msg.c_str());
							return std::nullopt;
						}
					}
					loadedShader = std::make_shared<MeshShader>
						(
							mesh.value(),
							task.has_value() ? task.value() : nullptr,
							frag.has_value() ? frag.value() : nullptr,
							preParsedInfo,
							filePath,
							this,
							preParsedInfo.renderPassFlags,
							shaderID
						);
					break;
				}
			case ShaderType::Compute:
			{
					auto comp = CompileGvkShader(preParsedInfo.vertexShader, filePath, "comp", workingDirectoryCStr, msg);
					if (!comp.has_value())
					{
						KBS_WARN("fail to compile compute shader in {} reason {}", filePath.c_str(), msg.c_str());
						return std::nullopt;
					}
					loadedShader = std::make_shared<ComputeShader>(comp.value(), filePath, this, shaderID);
					break;
				}
		}

		if (!loadedShader->GenerateReflection())
		{
			return std::nullopt;
		}
		m_Shaders[shaderID] = loadedShader;
		m_ShaderPathTable[absolutePath] = shaderID;
		
		return loadedShader;
	}

	opt<ptr<Shader>> kbs::ShaderManager::Get(const ShaderID& id)
	{
		if (m_Shaders.count(id))
		{
			return m_Shaders[id];
		}
		return  std::nullopt;
	}

	bool ShaderManager::Exists(const std::string& filePath)
	{
		std::string validAbsolutePath;
		fs::path inputFilePath = filePath;
		fs::path absoluteInputFilePath = fs::absolute(inputFilePath);

		if (m_ShaderPathTable.count(absoluteInputFilePath.string()))
		{
			return true;
		}
		else if (inputFilePath.is_relative())
		{
			for (auto s : m_SearchingDirectories)
			{
				fs::path searchPath = s / inputFilePath;
				return m_ShaderPathTable.count(searchPath.string());
			}
		}
		return false;
	}

	opt<ShaderInfo> ShaderManager::LoadAndPreParse(const std::string& filePath, std::string& fileabsolutePath)
	{
		std::string validAbsolutePath;
		fs::path inputFilePath = filePath;
		if (fs::exists(inputFilePath))
		{
			if (inputFilePath.is_absolute())
			{
				validAbsolutePath = inputFilePath.string();
			}
			else
			{
				validAbsolutePath = fs::absolute(inputFilePath).string();
			}
		}
		else if(inputFilePath.is_relative())
		{
			for (auto s : m_SearchingDirectories)
			{
				fs::path searchPath = s / inputFilePath;
				if (fs::exists(searchPath))
				{
					validAbsolutePath = searchPath.string();
					break;
				}
			}
		}
		if (validAbsolutePath.empty())
		{
			KBS_WARN("file {} is invalid", filePath.c_str());
			return std::nullopt;
		}

		std::ifstream inf(validAbsolutePath);
		inf.seekg(0, std::ios::end);
		size_t fileSize = inf.tellg();
		inf.seekg(0, std::ios::beg);

		std::string str(fileSize, ' ');
		inf.read(str.data(), fileSize);

		std::string msg;
		opt<ShaderInfo> info = kbs::ShaderParser::Parse(str, &msg);
		if (!info.has_value())
		{
			KBS_WARN("fail to preparse shader file {} reason : {}", validAbsolutePath.c_str(), msg.c_str());
		}

		fileabsolutePath = validAbsolutePath;

		return info;
	}

	opt<ptr<gvk::Shader>> ShaderManager::CompileGvkShader(const std::string& preParsedShaderContent, const std::string& shaderFilePath, const std::string& target, 
		std::vector<const char*>& workingDirectories, std::string& msg)
	{
		std::string shaderFileName = shaderFilePath + "." + target;
		std::ofstream ouf(shaderFileName, std::ofstream::out | std::ofstream::trunc);
		ouf.write(preParsedShaderContent.c_str(), preParsedShaderContent.size());
		ouf.close();

		auto res = m_Context->CompileShader(shaderFileName.c_str(), gvk::ShaderMacros(),
			workingDirectories.data(), workingDirectories.size(), workingDirectories.data(), workingDirectories.size(), &msg);

		return res;
	}


	ShaderType Shader::GetShaderType()
	{
		return m_ShaderType;
	}

	std::string kbs::Shader::GetShaderPath()
	{
		return m_ShaderPath;
	}

	ShaderID Shader::GetShaderID()
	{
		return m_Id;
	}

	ShaderReflection& Shader::GetShaderReflection()
	{
		return m_Reflection;
	}


	kbs::Shader::Shader(ShaderType type, std::string shaderPath, ShaderManager* manager, ShaderID id)
		:m_ShaderType(type), m_ShaderPath(shaderPath),m_Manager(manager),m_Id(id) {}

	void SurfaceShader::OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info)
	{
		AssignRFDState(info);
		info.fragment_shader = frag;
		info.vertex_shader = m_Manager->m_StandardVertexShader;
	}

	bool SurfaceShader::GenerateReflection()
	{
		ptr<gvk::Shader> vert = m_Manager->m_StandardVertexShader;
		
		std::vector<SpvReflectDescriptorBinding*> vert_bindings = vert->GetDescriptorBindings().value();
		
		std::vector<SpvReflectDescriptorBinding*> frag_bindings;
		if (frag != nullptr)
		{
			frag_bindings = frag->GetDescriptorBindings().value();
		}

		vert_bindings.insert(vert_bindings.end(), frag_bindings.begin(), frag_bindings.end());

		std::string msg;
		bool success = m_Reflection.GraphicsReflectionFromBindings(vert_bindings, msg);
		if (!success)
		{
			KBS_WARN("fail to generate reflection for shader {} reason {}", m_ShaderPath.c_str(), msg.c_str());
			return false;
		}

		return true;
	}

	void CustomVertexShader::OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info)
	{
		AssignRFDState(info);
		info.vertex_shader = vert;
		info.fragment_shader = frag;
	}

	bool CustomVertexShader::GenerateReflection()
	{
		std::vector<SpvReflectDescriptorBinding*> vert_bindings = vert->GetDescriptorBindings().value();
		std::vector<SpvReflectDescriptorBinding*> frag_bindings;
		if (frag != nullptr)
		{
			frag_bindings = frag->GetDescriptorBindings().value();
		}

		vert_bindings.insert(vert_bindings.end(), frag_bindings.begin(), frag_bindings.end());

		std::string msg;
		bool success = m_Reflection.GraphicsReflectionFromBindings(vert_bindings, msg);
		if (!success)
		{
			KBS_WARN("fail to generate reflection for shader {} reason {}", m_ShaderPath.c_str(), msg.c_str());
			return false;
		}

		return true;
	}


	void MeshShader::OnPipelineStateCreate(GvkGraphicsPipelineCreateInfo& info)
	{
		AssignRFDState(info);
		info.mesh_shader = mesh;
		info.task_shader = task;
		info.fragment_shader = frag;
	}

	bool MeshShader::GenerateReflection()
	{
		std::vector<SpvReflectDescriptorBinding*> frag_bindings;
		if (frag != nullptr)
		{
			frag_bindings = frag->GetDescriptorBindings().value();
		}
		std::vector<SpvReflectDescriptorBinding*> task_bindings;
		if (task != nullptr)
		{
			task_bindings = task->GetDescriptorBindings().value();
		}
		std::vector<SpvReflectDescriptorBinding*> mesh_bindings;
		if (mesh != nullptr)
		{
			mesh_bindings = task->GetDescriptorBindings().value();
		}
		mesh_bindings.insert(mesh_bindings.end(), task_bindings.begin(), task_bindings.end());
		mesh_bindings.insert(mesh_bindings.end(), task_bindings.begin(), task_bindings.end());

		std::string msg;
		bool success = m_Reflection.GraphicsReflectionFromBindings(mesh_bindings, msg);
		if (!success)
		{
			KBS_WARN("fail to generate reflection for shader {} reason {}", m_ShaderPath.c_str(), msg.c_str());
			return false;
		}

		return true;
	}

	void ComputeShader::OnPipelineStateCreate(GvkComputePipelineCreateInfo& info)
	{
		info.shader = comp;
	}

	bool ComputeShader::GenerateReflection()
	{
		std::vector<SpvReflectDescriptorBinding*> bindings = comp->GetDescriptorBindings().value();
		std::string msg;
		bool success = m_Reflection.ComputeReflectionFromBindings(bindings, msg);
		if (!success) 
		{
			KBS_WARN("fail to generate reflection for shader {} reason {}", m_ShaderPath.c_str(), msg.c_str());
			return false;
		}
		return true;
	}

	RenderPassFlags GraphicsShader::GetRenderPassFlags()
	{
		return m_RenderPassFlags;
	}

	void GraphicsShader::AssignRFDState(GvkGraphicsPipelineCreateInfo& info)
	{
		info.depth_stencil_state = depthStencilState;
		info.frame_buffer_blend_state = blendState;
		info.rasterization_state = raster;
	}

	void GraphicsShader::LoadShaderInfo(ShaderInfo& info, uint32_t blendLocationCount)
	{
		raster = info.raster;
		depthStencilState = info.depthStencilState;

		GvkGraphicsPipelineCreateInfo::FrameBufferBlendState fbs = info.blendState;
		fbs.Resize(blendLocationCount);
		for (auto& bs : info.blendStateArray)
		{
			if (bs.targetLocation < blendLocationCount)
			{
				blendState.Set(bs.targetLocation, bs.blendState);
			}
		}

		this->blendState = fbs;
	}

	// TODO support these flags
	constexpr SpvReflectTypeFlags unsupportedMemberTypeFlags = SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER |
		SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE | SPV_REFLECT_TYPE_FLAG_EXTERNAL_ACCELERATION_STRUCTURE
		| SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK | SPV_REFLECT_TYPE_FLAG_EXTERNAL_MASK | SPV_REFLECT_TYPE_FLAG_STRUCT
		| SPV_REFLECT_TYPE_FLAG_ARRAY;

	bool ShaderReflection::GraphicsReflectionFromBindings(std::vector<SpvReflectDescriptorBinding*>& bindings, std::string& msg)
	{
		struct VariableNameInfo
		{
			std::string name;
			VariableInfo info;
		};

		m_VariableBufferSize = 0;
		for (auto& b : bindings)
		{
			if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				&& b->set == (uint32_t)ShaderSetUsage::perMaterial
				&& b->binding == 0)
			{
				std::vector<VariableNameInfo> structMemberInfos;
				for (uint32_t i = 0; i != b->type_description->member_count; i++)
				{
					auto& member = b->type_description->members[i];
					if (kbs_contains_flags(member.type_flags, unsupportedMemberTypeFlags))
					{
						msg = std::string("member ") + member.struct_member_name + "'s type is not supported";
						return false;
					}

					VariableInfo info;
					info.binding = b->binding;
					info.set = b->set;

					if (kbs_contains_flags(member.type_flags, SPV_REFLECT_TYPE_FLAG_MATRIX))
					{
						if (!kbs_contains_flags(member.type_flags, SPV_REFLECT_TYPE_FLAG_FLOAT) || member.traits.numeric.matrix.row_count != 4 ||
							member.traits.numeric.matrix.column_count != 4)
						{
							msg = "currently only float matrix 4x4 is supported!";
							return false;
						}
						info.type = VariableType::Mat4;
						info.stride = sizeof(float) * 16;
					}
					else if (kbs_contains_flags(member.type_flags, SPV_REFLECT_TYPE_FLAG_VECTOR))
					{
						if (!kbs_contains_flags(member.type_flags, SPV_REFLECT_TYPE_FLAG_FLOAT))
						{
							msg = "currently only float vector is supported!";
							return false;
						}

						
						switch (member.traits.numeric.vector.component_count)
						{
						case 1:
							info.type = VariableType::Float;
							info.stride = sizeof(float);
							break;
						case 2:
							info.type = VariableType::Float2;
							info.stride = sizeof(float) * 2;
							break;
						case 3:
							info.type = VariableType::Float3;
							info.stride = sizeof(float) * 3;
							break;
						case 4:
							info.type = VariableType::Float4;
							info.stride = sizeof(float) * 4;
							break;
						}
					}
					else if (kbs_contains_flags(member.type_flags, SPV_REFLECT_TYPE_FLAG_FLOAT))
					{
						info.type = VariableType::Float;
						info.stride = sizeof(float);
					}
					else if (kbs_contains_flags(member.type_flags, SPV_REFLECT_TYPE_FLAG_INT))
					{
						info.type = VariableType::Int;
						info.stride = sizeof(int);
					}

					VariableNameInfo namedInfo;
					namedInfo.name = member.struct_member_name;
					namedInfo.info = info;
					structMemberInfos.push_back(namedInfo);
				}

				uint32_t alignedBufferSize = 0;
				for (uint32_t i = 0; i < structMemberInfos.size(); i++)
				{
					structMemberInfos[i].info.offset = alignedBufferSize;
					alignedBufferSize += structMemberInfos[i].info.stride;

					if (i != structMemberInfos.size() - 1)
					{
						/*
							opengl std143 specifies
							1. If the member is a scalar consuming N basic machine units, the base alignment is N.
							2. If the member is a two - or four - component vector with components consuming N basic machine units, the base alignment is 2N or 4N, respectively.
							3. If the member is a three - component vector with components consuming N basic machine units, the base alignment is 4N.
						*/
						switch (structMemberInfos[i + 1].info.type)
						{
						case VariableType::Int:
						case VariableType::Float:
							break;
						case VariableType::Float2:
							alignedBufferSize = round_up<8>(alignedBufferSize);
							break;
						case VariableType::Float3:
							alignedBufferSize = round_up<16>(alignedBufferSize);
							break;
						case VariableType::Float4:
							alignedBufferSize = round_up<16>(alignedBufferSize);
							break;
						case VariableType::Mat4:
							alignedBufferSize = round_up<16>(alignedBufferSize);
							break;
						}
					}

				}

				for (uint32_t i = 0; i < structMemberInfos.size(); i++)
				{
					m_VariableInfos[structMemberInfos[i].name] = structMemberInfos[i].info;
				}
				m_VariableBufferSize = alignedBufferSize;
			}
			else if (b->set == (uint32_t)ShaderSetUsage::perObject)
			{
				if (b->binding != 0 || b->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					msg = "invalid shader binding at set perObject binding " + std::to_string(b->binding) + " only binding 0 is allowed at set perObject";
					return false;
				}
				if (std::string(b->name) != "object")
				{
					msg = "invalid shader binding name at set perObject binding " + std::string(b->name) + " name must be 'object'";
					return false;
				}

			}
			else if (b->set == (uint32_t)ShaderSetUsage::perCamera)
			{
				if (b->binding != 0 || b->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					msg = "invalid shader binding at set perCamera binding " + std::to_string(b->binding) + " only binding 0 is allowed at set perCamera";
					return false;
				}
				if (std::string(b->name) != "camera")
				{
					msg = "invalid shader binding at set perCamera binding " + std::to_string(b->binding) + " name must be 'camera'";
					return false;
				}
			}
			else if(b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				TextureInfo info;
				info.binding = b->binding;
				info.set = b->set;
				info.depth = b->image.depth;
				info.dim = b->image.dim;

				m_TextureInfos[b->name] = info;
			}
			else if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				|| b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				BufferInfo info;
				info.binding = b->binding;
				info.set = b->set;
				info.type = b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? BufferType::Uniform : BufferType::Storage;

				m_BufferInfos[b->name] = info;
			}
			else
			{
				msg = "currently binding at set " + std::to_string(b->set) + " binding " + std::to_string(b->binding) + " name " + b->name + " is not supported";
				return false;
			}
		}

		return true;
	}

	bool ShaderReflection::ComputeReflectionFromBindings(std::vector<SpvReflectDescriptorBinding*>& bindings, std::string& msg)
	{
		m_VariableBufferSize = 0;
		for (auto& b : bindings)
		{
			if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				TextureInfo info;
				info.binding = b->binding;
				info.set = b->set;
				info.depth = b->image.depth;
				info.dim = b->image.dim;

				m_TextureInfos[b->name] = info;
			}
			else if (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				|| b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				BufferInfo info;
				info.binding = b->binding;
				info.set = b->set;

				m_BufferInfos[b->name] = info;
			}
			else
			{
				msg = "currently binding at set " + std::to_string(b->set) + " binding " + std::to_string(b->binding) + " name " + b->name + " is not supported";
				return false;
			}
		}

		return true;
	}


	opt<ShaderReflection::VariableInfo> ShaderReflection::GetVariable(const std::string& name)
	{
		if (!m_VariableInfos.count(name))
		{
			return std::nullopt;
		}
		return m_VariableInfos[name];
	}
	uint32_t ShaderReflection::GetVariableBufferSize()
	{
		return m_VariableBufferSize;
	}
	opt<ShaderReflection::BufferInfo> ShaderReflection::GetBuffer(const std::string& name)
	{
		if (!m_BufferInfos.count(name))
		{
			return std::nullopt;
		}
		return m_BufferInfos[name];
	}
	opt<ShaderReflection::TextureInfo> ShaderReflection::GetTexture(const std::string& name)
	{
		if (!m_TextureInfos.count(name))
		{
			return std::nullopt;
		}
		return m_TextureInfos[name];
	}
}

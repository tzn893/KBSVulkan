#include "ShaderParser.h"
#include "Common.h"

namespace kbs
{
	enum class TokenType
	{
		Void,
		None,
		VertexBegin,
		VertexEnd,
		FragmentBegin,
		FragmentEnd,
		MeshBegin,
		MeshEnd,
		TaskBegin,
		TaskEnd,
		ComputeBegin,
		ComputeEnd,
		Include,
		ZWrite,
		RayGenBegin,
		RayGenEnd,
		HitGroupBegin,
		HitGroupEnd,
		ClosestHitBegin,
		ClosestHitEnd,
		AnyHitBegin,
		AnyHitEnd,
		IntersectionBegin,
		IntersectionEnd,
		MissBegin,
		MissEnd
	};

	TokenType ParseToken(std::string token)
	{
		std::string stripedToken = string_strip(token);
		if (stripedToken.empty()) return TokenType::Void;
		else if (stripedToken == "#pragma kbs_vertex_begin")
		{
			return TokenType::VertexBegin;
		}
		else if (stripedToken == "#pragma kbs_vertex_end")
		{
			return TokenType::VertexEnd;
		}
		else if (stripedToken == "#pragma kbs_fragment_begin")
		{
			return TokenType::FragmentBegin;
		}
		else if (stripedToken == "#pragma kbs_fragment_end")
		{
			return TokenType::FragmentEnd;
		}
		else if (stripedToken == "#pragma kbs_task_begin")
		{
			return TokenType::TaskBegin;
		}
		else if (stripedToken == "#pragma kbs_task_end")
		{
			return TokenType::TaskEnd;
		}
		else if (stripedToken == "#pragma kbs_mesh_begin")
		{
			return TokenType::MeshBegin;
		}
		else if (stripedToken == "#pragma kbs_mesh_end")
		{
			return TokenType::MeshEnd;
		}
		else if (stripedToken == "#pragma kbs_compute_begin")
		{
			return TokenType::ComputeBegin;
		}
		else if (stripedToken == "#pragma kbs_compute_end")
		{
			return TokenType::ComputeEnd;
		}
		else if (stripedToken == "#pragma kbs_ray_gen_begin")
		{
			return TokenType::RayGenBegin;
		}
		else if (stripedToken == "#pragma kbs_ray_gen_end")
		{
			return TokenType::RayGenEnd;
		}
		else if (stripedToken == "#pragma kbs_hit_group_begin")
		{
			return TokenType::HitGroupBegin;
		}
		else if (stripedToken == "#pragma kbs_hit_group_end")
		{
			return TokenType::HitGroupEnd;
		}
		else if (stripedToken == "#pragma kbs_closest_hit_begin")
		{
			return TokenType::ClosestHitBegin;
		}
		else if (stripedToken == "#pragma kbs_closest_hit_end")
		{
			return TokenType::ClosestHitEnd;
		}
		else if (stripedToken == "#pragma kbs_any_hit_begin")
		{
			return TokenType::AnyHitBegin;
		}
		else if (stripedToken == "#pragma kbs_any_hit_end")
		{
			return TokenType::AnyHitEnd;
		}
		else if (stripedToken == "#pragma kbs_intersection_begin")
		{
			return TokenType::IntersectionBegin;
		}
		else if (stripedToken == "#pragma kbs_intersection_end")
		{
			return TokenType::IntersectionEnd;
		}
		else if (stripedToken == "#pragma kbs_miss_begin")
		{
			return TokenType::MissBegin;
		}
		else if (stripedToken == "#pragma kbs_miss_end")
		{
			return TokenType::MissEnd;
		}
		else
		{
			std::vector<std::string> splitedStripedToken = string_split(stripedToken,' ');
			if (splitedStripedToken.size() > 1)
			{
				if (splitedStripedToken[1] == "include")
				{
					return TokenType::Include;
				}
				else if(splitedStripedToken[1] == "zwrite")
				{
					return TokenType::ZWrite;
				}
			}
		}

		return TokenType::None;
	}


	opt<std::string> ParseVertexShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::VertexEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_vertex_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_vertex_end";
		return std::nullopt;
	}

	opt<std::string> ParseAnyHitShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::AnyHitEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_any_hit_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_any_hit_end";
		return std::nullopt;
	}

	opt<std::string> ParseClosestHitShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::ClosestHitEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_closest_hit_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_closest_hit_end";
		return std::nullopt;
	}

	opt<std::string> ParseIntersectionShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::IntersectionEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_intersection_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_intersection_end";
		return std::nullopt;
	}

	opt<ShaderInfo::HitGroup> ParseHitGroup(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		ShaderInfo::HitGroup hitGroup;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p++]))
			{
				case TokenType::Void:
					break;
				case TokenType::HitGroupEnd:
					return hitGroup;
				case TokenType::AnyHitBegin:
					if (auto var = ParseAnyHitShader(p,tokens,err); var.has_value())
					{
						hitGroup.anyHitShader = var.value();
						break;
					}
					else
					{
						return std::nullopt;
					}
				case TokenType::ClosestHitBegin:
					if (auto var = ParseClosestHitShader(p, tokens, err);var.has_value())
					{
						hitGroup.cloestHitShader = var.value();
						break;
					}
					else
					{
						return std::nullopt;
					}
				case TokenType::IntersectionBegin:
					if (auto var = ParseIntersectionShader(p, tokens,err);var.has_value())
					{
						hitGroup.intersectionShader = var.value();
						break;
					}
					else
					{
						return std::nullopt;
					}

				default:
					*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect closest hit, any hit or intersection shader begin";
					return std::nullopt;
			}
			
		}
		return hitGroup;
	}

	opt<std::string> ParseMissShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::MissEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_miss_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_intersection_end";
		return std::nullopt;
	}

	opt<std::string> ParseRayGenShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::RayGenEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_ray_gen_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_intersection_end";
		return std::nullopt;
	}

	opt<std::string> ParseFragmentShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::FragmentEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_fragment_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_fragment_end";
		return std::nullopt;
	}

	opt<std::string> ParseMeshShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::MeshEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_mesh_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_mesh_end";
		return std::nullopt;
	}

	opt<std::string> ParseTaskShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::TaskEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_task_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_task_end";
		return std::nullopt;
	}

	opt<std::string> ParseComptueShader(uint32_t& p, std::vector<std::string>& tokens, std::string* err)
	{
		std::string res;
		while (p < tokens.size())
		{
			switch (ParseToken(tokens[p]))
			{
			case TokenType::Void:
			case TokenType::None:
				res += tokens[p] + "\n";
				break;
			case TokenType::ComputeEnd:
				p++;
				return res;
			default:
				*err = " invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_compute_end";
				return std::nullopt;
			}
			p++;
		}

		*err = "reach EOF expect #pragma kbs_compute_end";
		return std::nullopt;
	}

	opt<std::string> ParseInclude(std::string token, std::string* err)
	{
		std::vector<std::string> splitedIncludeStr = string_split(string_strip(token), ' ');
		if (splitedIncludeStr.size() != 3) 
		{
			*err = "invalid usage for include expect : #pragma include [included file name]";
			return std::nullopt;
		}
		return splitedIncludeStr[2];
	}

	opt<bool> ParseZWrite(std::string token, std::string* err)
	{
		std::vector<std::string> splitedIncludeStr = string_split(string_strip(token), ' ');
		if (splitedIncludeStr.size() != 3)
		{
			*err = "invalid usage for include expect : #pragma zwrite on/off";
			return std::nullopt;
		}
		if (splitedIncludeStr[2] == "on")
		{
			return true;
		}
		else if (splitedIncludeStr[2] == "off")
		{
			return false;
		}
		
		*err = "invalid option for zwrite : " + splitedIncludeStr[2];
		return std::nullopt;
	}

	opt<ShaderInfo> kbs::ShaderParser::Parse(const std::string& content, std::string* msg)
	{
		if (content.empty()) return std::nullopt;

		ShaderInfo info;
		info.depthStencilState.enable_depth_stencil = true;
		std::vector<std::string> tokens = string_split(content, '\n');
		if (string_strip(tokens[0]) != "#pragma kbs_shader")
		{
			info.type = ShaderType::Surface;
			info.fragmentShader = content;
			info.renderPassFlags = RenderPass_Opaque;

			return info;
		}
		
		
		uint32_t p = 1;
		while (p < tokens.size())
		{
			opt<std::string> res;
			switch (ParseToken(tokens[p++]))
			{
			case TokenType::Void:
				break;
			case TokenType::VertexBegin:
				res = ParseVertexShader(p, tokens, msg);
				if (res.has_value())
				{
					info.vertexShader = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::FragmentBegin:
				res = ParseFragmentShader(p, tokens, msg);
				if (res.has_value())
				{
					info.fragmentShader = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::TaskBegin:
				res = ParseTaskShader(p, tokens, msg);
				if (res.has_value())
				{
					info.taskShader = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::MeshBegin:
				res = ParseMeshShader(p, tokens, msg);
				if (res.has_value())
				{
					info.meshShader = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::ComputeBegin:
				res = ParseComptueShader(p, tokens, msg);
				if (res.has_value())
				{
					info.computeShader = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::MissBegin:
				res = ParseMissShader(p, tokens, msg);
				if (res.has_value())
				{
					info.rayMissShader.push_back(res.value());
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::RayGenBegin:
				res = ParseRayGenShader(p, tokens, msg);
				if (res.has_value())
				{
					info.rayGenShader = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::HitGroupBegin:
			{
				auto hitGroupShader = ParseHitGroup(p, tokens, msg);
				if (hitGroupShader.has_value())
				{
					info.hitGroupShader.push_back(hitGroupShader.value());
				}
				else
				{
					return std::nullopt;
				}
				break;
			}

			case TokenType::Include:
				res = ParseInclude(tokens[p - 1], msg);
				if (res.has_value())
				{
					info.includeFileList.push_back(res.value());
				}
				else
				{
					return std::nullopt;
				}
				break;
			case TokenType::ZWrite:
			{
				auto res = ParseZWrite(tokens[p - 1], msg);
				if (res.has_value())
				{
					info.depthStencilState.enable_depth_stencil = res.value();
				}
				else
				{
					return std::nullopt;
				}
				break;
			}
			default:
				*msg = "invalid token " + tokens[p] + " in line " + std::to_string(p) + " expect #pragma kbs_[shader type]_begin";
				return std::nullopt;
			}
		}

		if (!info.vertexShader.empty())
		{
			if (!info.taskShader.empty() || !info.meshShader.empty() || !info.computeShader.empty())
			{
				*msg = "vertex shader detected other shaders will be ignored";
			}
			info.type = ShaderType::CustomVertex;
		}
		else if (!info.meshShader.empty())
		{
			if (!info.vertexShader.empty() || !info.computeShader.empty())
			{
				*msg = "vertex shader detected other shaders will be ignored";
			}
			info.type = ShaderType::MeshShader;
		}
		else if (!info.taskShader.empty())
		{
			*msg = "no corresponding mesh shader with task shader";
			return std::nullopt;
		}
		else if (!info.computeShader.empty())
		{
			info.type = ShaderType::Compute;
		}
		else if(!info.fragmentShader.empty())
		{
			info.type = ShaderType::Surface;
		}
		else if (!info.rayGenShader.empty())
		{
			info.type = ShaderType::RayTracing;
		}
		else
		{
			*msg = "no shader code in this shader file";
			return std::nullopt;
		}

		// TODO parse shader's render pass flag
		info.renderPassFlags = RenderPass_Opaque;

		return info;
	}
}


#include "Renderer/ShaderParser.h"
#include "Renderer/Flags.h"
#include "gtest/gtest.h"

TEST(TestShader, SurfaceShader)
{
	std::string c1 =
		"#pragma once\n"
		"void main(){\n"
		"\n"
		"}\n"
	;
	std::string c2 =
		"slskskksksks\n"
		"sskskksks\n"
		"skskskodakodaskdosad\n"
		"skdsaaskdosakdosakdosada\n"
		"sdkaoskdosadsad\n"
		"sdkaokdsaodkasd\n";
	std::string c3 =
		"#pragma kbs_shader";

	std::string c4 = "";


	std::string _;
	auto r1 = kbs::ShaderParser::Parse(c1, &_), 
		r2 = kbs::ShaderParser::Parse(c2, &_),
		r3 = kbs::ShaderParser::Parse(c3, &_), 
		r4 = kbs::ShaderParser::Parse(c4, &_);

	ASSERT_TRUE(r1.has_value());
	ASSERT_TRUE(r2.has_value());
	ASSERT_FALSE(r3.has_value());
	ASSERT_FALSE(r4.has_value());

	ASSERT_EQ(r1.value().type, kbs::ShaderType::Surface);
	ASSERT_EQ(r2.value().type, kbs::ShaderType::Surface);
}


#define s1 "void main(){\n      \n gl_Position = cameraUBO.camera.projection \n * cameraUBO.camera.view * objectUBO.object.model * tmpPos;\n }\n"
#define s2 "void main(){\n gl_Position = vec4(1, 0, 1, 1);\n      \n }\n"
#define s3 "layout (location = 0) out vec3 outNormal;\n layout(location = 1) out vec2 outUV;\n layout(location = 2) out vec3 outWorldPos; \n layout(location = 3) out vec3 outTangent;\n void main(){}\n"
#define s4 "void main(){\n mat3 mNormal = mat3(objectUBO.object.invTransModel);\n }\n"
#define s5 "void main(){\n outNormal = normalize(mNormal * inNormal);\n }\n"


/*
	   custom vertex
	   #pragma kbs_shader
	   #pragma kbs_vertex_begin

	   #pragma kbs_vertex_end

	   #pragma kbs_fragment_begin

	   #pragma kbs_fragment_end


	   mesh shader
	   #pragma kbs_shader
	   #pragma kbs_task_begin

	   #pragma kbs_task_end

	   #pragma kbs_mesh_begin
	   #pragma kbs_mesh_end

	   #pragma kbs_fragment_begin
	   #pragma kbs_fragment_end

	   compute shader
	   #pragma kbs_shader
	   #pragma kbs_compute_begin

	   #pragma kbs_compute_end
	*/

TEST(TestShader, SuccessMultiShader)
{
	std::string c1 =
		"#pragma kbs_shader\n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n";

	std::string _;
	auto r1 = kbs::ShaderParser::Parse(c1, &_);
	ASSERT_TRUE(r1.has_value());
	ASSERT_EQ(r1.value().type, kbs::ShaderType::Surface);
	ASSERT_EQ(r1.value().fragmentShader, s1);

	std::string c2 =
		"#pragma kbs_shader\n"
		"  \n"
		"    \n\n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r2 = kbs::ShaderParser::Parse(c2, &_);
	ASSERT_TRUE(r2.has_value());
	ASSERT_EQ(r2.value().type, kbs::ShaderType::CustomVertex);
	ASSERT_EQ(r2.value().fragmentShader, s1);
	ASSERT_EQ(r2.value().vertexShader, s2);

	std::string c7 =
		"#pragma kbs_shader\n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end"
		;
	auto r7 = kbs::ShaderParser::Parse(c7, &_);
	ASSERT_TRUE(r7.has_value());
	ASSERT_EQ(r7.value().type, kbs::ShaderType::CustomVertex);
	ASSERT_EQ(r7.value().vertexShader, s2);

	std::string c3 =
		"#pragma kbs_shader\n"
		"   \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		;
	auto r3 = kbs::ShaderParser::Parse(c3, &_);
	ASSERT_TRUE(r3.has_value());
	ASSERT_EQ(r3.value().type, kbs::ShaderType::CustomVertex);
	ASSERT_EQ(r3.value().fragmentShader, s1);
	ASSERT_EQ(r3.value().vertexShader, s2);


	std::string c4 = 
		"#pragma kbs_shader\n"
		"#pragma kbs_mesh_begin \n"
		s3
		"#pragma kbs_mesh_end \n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		;
	auto r4 = kbs::ShaderParser::Parse(c4, &_);
	ASSERT_TRUE(r4.has_value());
	ASSERT_EQ(r4.value().type, kbs::ShaderType::MeshShader);
	ASSERT_EQ(r4.value().fragmentShader, s1);
	ASSERT_EQ(r4.value().meshShader, s3);


	std::string c5 =
		"#pragma kbs_shader\n"
		"#pragma kbs_mesh_begin \n"
		s3
		"#pragma kbs_mesh_end \n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_task_begin \n"
		s4
		"#pragma kbs_task_end \n"
		;
	auto r5 = kbs::ShaderParser::Parse(c5, &_);
	ASSERT_TRUE(r5.has_value());
	ASSERT_EQ(r5.value().type, kbs::ShaderType::MeshShader);
	ASSERT_EQ(r5.value().fragmentShader, s1);
	ASSERT_EQ(r5.value().meshShader, s3);
	ASSERT_EQ(r5.value().taskShader, s4);

	std::string c6 =
		"#pragma kbs_shader\n"
		"#pragma kbs_compute_begin\n"
		s5
		"#pragma kbs_compute_end\n"
		;
	auto r6 = kbs::ShaderParser::Parse(c6, &_);
	ASSERT_TRUE(r6.has_value());
	ASSERT_EQ(r6.value().type, kbs::ShaderType::Compute);
	ASSERT_EQ(r6.value().computeShader, s5);
}

TEST(TestShader, FailMultiShader)
{
	std::string _;
	std::string c1 =
		"#pragma kbs_shader\n"
		"#pragma kbs_fragment_begin \n"
		s1;
	ASSERT_FALSE(kbs::ShaderParser::Parse(c1, &_).has_value());

	std::string c2 =
		"#pragma kbs_shader";
	ASSERT_FALSE(kbs::ShaderParser::Parse(c2, &_).has_value());

	std::string c3 =
		"#pragma kbs_shader\n"
		s1
		"#pragma kbs_mesh_end";
	ASSERT_FALSE(kbs::ShaderParser::Parse(c3, &_).has_value());

	std::string c4 = 
		"#pragma kbs_shader\n"
		"#pragma kbs_mesh_begin \n"
		s3
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_task_begin \n"
		s4;
	ASSERT_FALSE(kbs::ShaderParser::Parse(c4, &_).has_value());

	std::string c5 =
		"#pragma kbs_shader\n"
		"#pragma kbs_fragment_begin \n"
		s3
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_task_begin \n"
		s4
		"#pragma kbs_task_end \n"
	;
	ASSERT_FALSE(kbs::ShaderParser::Parse(c5, &_).has_value());

	std::string c6 =
		"#pragma kbs_shader\n\n"
		"#pragma kbs_ \n"
		s3
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_task_begin \n"
		s4
		"#pragma kbs_task_end \n";

	ASSERT_FALSE(kbs::ShaderParser::Parse(c6, &_).has_value());
}


/*
#pragma zwrite off
#pragma zwrite on
#pragma include "..."
*/
TEST(TestShader, ShaderAttributes)
{
	std::string _;
	std::string c1 =
		"#pragma kbs_shader\n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r1 = kbs::ShaderParser::Parse(c1, &_);
	ASSERT_TRUE(r1.value().depthStencilState.enable_depth_stencil);



	std::string c2 =
		"#pragma kbs_shader\n"
		"#pragma    zwrite     off\n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r2 = kbs::ShaderParser::Parse(c2, &_);
	ASSERT_FALSE(r2.value().depthStencilState.enable_depth_stencil);

	std::string c3 =
		"#pragma kbs_shader\n"
		"#pragma    zwrite     on     \n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r3 = kbs::ShaderParser::Parse(c3, &_);
	ASSERT_TRUE(r3.value().depthStencilState.enable_depth_stencil);


	std::string c4 =
		"#pragma kbs_shader\n"
		"#pragma kbs_fragment_begin \n"
		"#pragma    zwrite     on     \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r4 = kbs::ShaderParser::Parse(c4, &_);
	ASSERT_FALSE(r4.has_value());

	std::string c5 = 
		"#pragma kbs_shader\n"
		"#pragma include a.glsli   \n"
		"#pragma include c:/d/e/f/g/b.glsli\n"
		"#pragma include f/some_file.glsli\n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r5 = kbs::ShaderParser::Parse(c5, &_);
	auto fileList = r5.value().includeFileList;
	ASSERT_EQ(fileList[0], "a.glsli");
	ASSERT_EQ(fileList[1], "c:/d/e/f/g/b.glsli");
	ASSERT_EQ(fileList[2], "f/some_file.glsli");


	std::string c6 =
		"#pragma kbs_shader\n"
		"#pragma kbs_fragment_begin \n"
		"#pragma    zwrite \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r6 = kbs::ShaderParser::Parse(c6, &_);
	ASSERT_FALSE(r6.has_value());

	std::string c7 =
		"#pragma kbs_shader\n"
		"#pragma include a.glsli  sdad \n"
		"#pragma kbs_fragment_begin \n"
		s1
		"#pragma kbs_fragment_end \n"
		"#pragma kbs_vertex_begin \n"
		s2
		"#pragma kbs_vertex_end \n"
		;
	auto r7 = kbs::ShaderParser::Parse(c7, &_);
	ASSERT_FALSE(r7.has_value());

	std::string c8 =
		"slskskksksks\n"
		"sskskksks\n"
		"skskskodakodaskdosad\n"
		"skdsaaskdosakdosakdosada\n"
		"sdkaoskdosadsad\n"
		"sdkaokdsaodkasd\n";
	auto r8 = kbs::ShaderParser::Parse(c8, &_);
	ASSERT_EQ(r8.value().renderPassFlags, kbs::RenderPass_Opaque);
}

int main() {
	testing::InitGoogleTest();
	RUN_ALL_TESTS();
}
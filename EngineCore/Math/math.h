#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kbs
{
	inline constexpr float pi = 3.1415926;
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;

	using mat2 = glm::mat2;
	using mat3 = glm::mat3;
	using mat4 = glm::mat4;

	using quat = glm::quat;

	struct Angle
	{
		float radius;
		Angle(float radius) : radius(radius) {}
		Angle() = default;

		static Angle FromRadius(float radius) { return Angle(radius); }
		static Angle FromDegree(float degree) { return Angle(degree / 180 * pi); }

		float GetRadius() { return radius; }
		float GetDegree() { return radius * 180 / pi; }
	};

	namespace math
	{
		inline float cos(Angle angle)
		{
			return ::cos(angle.radius);
		}

		inline float sin(Angle angle)
		{
			return ::sin(angle.radius);
		}

		inline float tan(Angle angle)
		{
			return ::tanf(angle.radius);
		}

		inline vec3 cross(vec3 lhs, vec3 rhs)
		{
			return glm::cross(lhs, rhs);
		}

		inline mat2 out_product(vec2 lhs, vec2 rhs)
		{
			return glm::outerProduct(lhs, rhs);
		}

		inline mat3 out_product(vec3 lhs, vec3 rhs)
		{
			return glm::outerProduct(lhs, rhs);
		}

		inline mat4 out_product(vec4 lhs, vec4 rhs)
		{
			return glm::outerProduct(lhs, rhs);
		}

		inline mat2 mul_element_wise(mat2 lhs, mat2 rhs)
		{
			return mat2(lhs[0][0] * rhs[0][0], lhs[0][1] * rhs[0][1], 
				        lhs[1][0] * rhs[1][0], lhs[1][1] * rhs[1][1]);
		}

		inline mat3 mul_element_wise(mat3 lhs, mat3 rhs)
		{
			return mat3(
				lhs[0][0] * rhs[0][0], lhs[0][1] * rhs[0][1], lhs[0][2] * rhs[0][2],
				lhs[1][0] * rhs[1][0], lhs[1][1] * rhs[1][1], lhs[1][2] * rhs[1][2],
				lhs[2][0] * rhs[2][0], lhs[2][1] * rhs[2][1], lhs[2][2] * rhs[2][2]
			);
		}

		inline mat4 mul_element_wise(mat4 lhs, mat4 rhs)
		{
			return mat4(
				lhs[0][0] * rhs[0][0], lhs[0][1] * rhs[0][1], lhs[0][2] * rhs[0][2], lhs[0][3] * rhs[0][3],
				lhs[1][0] * rhs[1][0], lhs[1][1] * rhs[1][1], lhs[1][2] * rhs[1][2], lhs[1][3] * rhs[1][3],
				lhs[2][0] * rhs[2][0], lhs[2][1] * rhs[2][1], lhs[2][2] * rhs[2][2], lhs[2][3] * rhs[2][3],
				lhs[3][0] * rhs[3][0], lhs[3][1] * rhs[3][1], lhs[3][2] * rhs[3][2], lhs[3][3] * rhs[3][3]
			);
		}

		inline mat2 inverse(mat2 m)
		{
			return glm::inverse(m);
		}

		inline mat3 inverse(mat3 m)
		{
			return glm::inverse(m);
		}

		inline mat4 inverse(mat4 m)
		{
			return glm::inverse(m);
		}

		inline mat2 transpose(mat2 m)
		{
			return glm::transpose(m);
		}

		inline mat3 transpose(mat3 m)
		{
			return glm::transpose(m);
		}

		inline mat4 transpose(mat4 m)
		{
			return glm::transpose(m);
		}

		inline quat mat2quat(mat4 r)
		{
			return glm::quat_cast(r);
		}

		inline mat4 quat2mat(quat q)
		{
			return glm::mat4_cast(q);
		}

		inline quat axisAngle(vec3 axis, Angle angle)
		{
			return glm::angleAxis(angle.radius, axis);
		}

		inline quat euler2Quat(vec3 euler)
		{
			return glm::quat(euler);
		}

		inline vec3 quat2Euler(quat q)
		{
			return glm::eulerAngles(q);
		}

		inline mat4 euler2Mat(vec3 euler)
		{
			return quat2mat(euler2Quat(euler));
		}
	}
}
#ifndef Renderer_ViewFrustum_hpp
#define Renderer_ViewFrustum_hpp

#include "Utility.hpp"

namespace GX
{

struct ViewFrustum
{
	glm::vec4 planes[6];
	glm::vec3 corners[8];
	glm::quat rotation;
	glm::vec3 position;

	ViewFrustum(glm::vec3 position, glm::quat rotation, float fov, float aspect, float near, float far) :
		position(position), 
		rotation(rotation)
	{
		//A really crappy way of constructing a view frustum
		glm::mat4 projectionMatrix = glm::perspective(fov, aspect, near, far);
		glm::mat4 viewMatrix = 
			glm::mat4_cast(glm::inverse(rotation)) * 
			glm::translate(glm::mat4(1.0f), -position);
		glm::mat4 projectionViewMatrix = projectionMatrix * viewMatrix;
		glm::mat4 inverseProjectionViewMatrix = glm::inverse(projectionViewMatrix);

		for (int z = 0; z < 2; z++)
		for (int y = 0; y < 2; y++)
		for (int x = 0; x < 2; x++)
		{
			int i = x + y * 2 + z * 2 * 2;
			corners[i] = glm::vec3(inverseProjectionViewMatrix * glm::vec4(x, y, z, 1));
		}
		
		for (int i = 0; i < 3; i++)
		{
			planes[2 * i + 0] = glm::row(projectionViewMatrix, 3) + glm::row(projectionViewMatrix, i);
			planes[2 * i + 1] = glm::row(projectionViewMatrix, 3) - glm::row(projectionViewMatrix, i);
		}
		
		//Normalization
		for (int i = 0; i < 6; i++)
			planes[i] /= -glm::length(glm::vec3(planes[i]));
	}

	bool containsPoint(glm::vec3 center) const 
	{
		for (int i = 0; i < 6; i++)
			if (glm::dot(planes[i], glm::vec4(center - position, 1.0f)) < 0.0)
				return false;
		return true;
	}

	bool overlapsBox(glm::vec3 center, glm::vec3 extents) const
	{
		//Intersection code from http://www.iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
		glm::vec3 min = center - extents;
		glm::vec3 max = center + extents;
		for(int i = 0; i < 6; i++ )
		{
			int out = 0;
			out += ((glm::dot( planes[i], glm::vec4(min.x, min.y, min.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(max.x, min.y, min.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(min.x, max.y, min.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(max.x, max.y, min.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(min.x, min.y, max.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(max.x, min.y, max.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(min.x, max.y, max.z, 1.0f) ) < 0.0 )?1:0);
			out += ((glm::dot( planes[i], glm::vec4(max.x, max.y, max.z, 1.0f) ) < 0.0 )?1:0);
			if(out == 8) return false;
		}
		int out;
		out=0; for(int i = 0; i < 8; i++) out += ((corners[i].x > max.x)?1:0); if(out == 8) return false;
		out=0; for(int i = 0; i < 8; i++) out += ((corners[i].x < min.x)?1:0); if(out == 8) return false;
		out=0; for(int i = 0; i < 8; i++) out += ((corners[i].y > max.y)?1:0); if(out == 8) return false;
		out=0; for(int i = 0; i < 8; i++) out += ((corners[i].y < min.y)?1:0); if(out == 8) return false;
		out=0; for(int i = 0; i < 8; i++) out += ((corners[i].z > max.z)?1:0); if(out == 8) return false;
		out=0; for(int i = 0; i < 8; i++) out += ((corners[i].z < min.z)?1:0); if(out == 8) return false;
		return true;
	}
};

}//namespace GX

#endif

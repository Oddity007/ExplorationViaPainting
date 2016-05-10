#ifndef Renderer_ViewFrustum_hpp
#define Renderer_ViewFrustum_hpp

#include "Utility.hpp"

namespace GX
{

/*struct Camera
{
	glm::vec3 position;
	GLfloat
		near,
		far,
		fov;
	glm::quat rotation;
	GLsizei
		resolutionWidth,
		resolutionHeight;

	enum class Type
	{
		Main,
	};

	Type type;
	
	Camera(void) :
		position(0, 0, 0), 
		rotation(1, 0, 0, 0), 
		near(1), 
		far(10000), 
		fov(45), 
		resolutionWidth(256),
		resolutionHeight(256),
		type(Type::Main)
	{
		
	}

	glm::mat4 calculateViewMatrix(void) const
	{
		return 
			glm::mat4_cast(glm::inverse(rotation)) * 
			glm::translate(glm::mat4(1.0f), -position);
	}

	glm::mat4 calculateProjectionMatrix(void) const
	{
		float aspect = resolutionWidth / (GLfloat) resolutionHeight;
		return glm::perspective(fov, aspect, near, far);
	}

	ViewFrustum calculateViewFrustum(void) const
	{
		return ViewFrustum(position, rotation, fov, resolutionWidth / float(resolutionHeight), near, far);
	}
};*/

}//namespace GX

#endif

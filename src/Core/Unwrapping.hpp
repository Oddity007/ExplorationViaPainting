#ifndef GX_Core_Unwrapping_hpp
#define GX_Core_Unwrapping_hpp

#include <vector>

namespace GX
{
namespace Unwrapping
{

struct Mesh
{
	struct Vertex
	{
		float
			position[3],
			qtangent[4];
		double texcoord[2];
		
		bool operator<(const Vertex& other) const
		{
			for (int i = 0; i < 3; i++)
				if (position[i] < other.position[i])
					return true;
			for (int i = 0; i < 4; i++)
				if (qtangent[i] < other.qtangent[i])
					return true;
			for (int i = 0; i < 2; i++)
				if (texcoord[i] < other.texcoord[i])
					return true;
			return false;
		}
	};
	std::vector<Vertex> vertices;
	std::vector<size_t> indices;
};

Mesh&& Unwrap(const Mesh& input);

}//namespace Unwrapping
}//namespace GX

#endif
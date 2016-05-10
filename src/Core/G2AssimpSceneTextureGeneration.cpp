#include "G2AssimpScene.h"
#include <CGAL/Polyhedral_mesh_domain_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Parameterization_polyhedron_adaptor_3.h>
#include <CGAL/parameterize.h>
#include <CGAL/Parameterization_mesh_patch_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <map>
#include "Core/Unwrapping.hpp"
#include <algorithm>

using Kernel = CGAL::Simple_cartesian<double>;
using Polyhedron = CGAL::Polyhedron_3<Kernel>;
using Parameterization_polyhedron_adaptor = CGAL::Parameterization_polyhedron_adaptor_3<Polyhedron>;
using Mesh_patch_polyhedron = CGAL::Parameterization_mesh_patch_3<Parameterization_polyhedron_adaptor>;
using Parameterizer = CGAL::Parameterizer_traits_3<Mesh_patch_polyhedron>;
using Seam = std::list<Parameterization_polyhedron_adaptor::Vertex_handle>;
using Mesh_feature_extractor = CGAL::Parameterization_mesh_feature_extractor<Parameterization_polyhedron_adaptor>;

static Seam cut_mesh(Parameterization_polyhedron_adaptor& mesh_adaptor)
{
    Seam seam;              // returned list
    // Get reference to Polyhedron_3 mesh
    Polyhedron& mesh = mesh_adaptor.get_adapted_mesh();
    // Extract mesh borders and compute genus
    Mesh_feature_extractor feature_extractor(mesh_adaptor);
    int nb_borders = feature_extractor.get_nb_borders();
    int genus = feature_extractor.get_genus();
    // If mesh is a topological disk
    if (genus == 0 && nb_borders > 0)
    {
        // Pick the longest border
        seam = feature_extractor.get_longest_border();
    }
    else // if mesh is *not* a topological disk, create a virtual cut
    {
        const int CUT_LENGTH = 6;
        // Build consecutive halfedges array
        Polyhedron::Halfedge_handle seam_halfedges[CUT_LENGTH];
        seam_halfedges[0] = mesh.halfedges_begin();
        if (seam_halfedges[0] == NULL)
            return seam;                    // return empty list
        int i;
        for (i=1; i<CUT_LENGTH; i++)
        {
            seam_halfedges[i] = seam_halfedges[i-1]->next()->opposite()->next();
            if (seam_halfedges[i] == NULL)
                return seam;                // return empty list
        }
        // Convert halfedges array to two-ways vertices list
        for (i=0; i<CUT_LENGTH; i++)
            seam.push_back(seam_halfedges[i]->vertex());
        for (i=CUT_LENGTH-1; i>=0; i--)
            seam.push_back(seam_halfedges[i]->opposite()->vertex());
    }
    return seam;
}

namespace
{
// A modifier creating a triangle with the incremental builder.
template <class HDS>
class Build_triangle : public CGAL::Modifier_base<HDS> {
public:
	const GX::Unwrapping::Mesh* input;
	
    Build_triangle(const GX::Unwrapping::Mesh* input) : input(input) {}
    void operator()( HDS& hds) {
        // Postcondition: hds is a valid polyhedral surface.
        CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
        B.begin_surface(input->vertices.size(), input->indices.size() / 3);
        typedef typename HDS::Vertex   Vertex;
        typedef typename Vertex::Point Point;
		for (size_t i = 0; i < input->vertices.size(); i++)
		{
			auto v = input->vertices[i];
			B.add_vertex(Point(v.position[0], v.position[1], v.position[2]));
		}
		for (size_t i = 0; i < input->indices.size(); i += 3)
		{
			B.begin_facet();
			for (size_t j = 0; j < 3; j++)
				B.add_vertex_to_facet(input->indices[i + j]);
			B.end_facet();
		}
        B.end_surface();
    }
};
}//namespace

GX::Unwrapping::Mesh&& GX::Unwrapping::Unwrap(const GX::Unwrapping::Mesh& input)
{
	assert(input.indices.size() % 3 == 0);
	
	Polyhedron mesh;
	
	{
		using HalfedgeDS = Polyhedron::HalfedgeDS;
		Build_triangle<HalfedgeDS> triangle(& input);
		mesh.delegate(triangle);
		CGAL_assertion(mesh.is_triangle(mesh.halfedges_begin()));
	}
	
	Parameterization_polyhedron_adaptor mesh_adaptor(mesh);
	Seam seam = cut_mesh(mesh_adaptor);
	Mesh_patch_polyhedron mesh_patch(mesh_adaptor, seam.begin(), seam.end());
	assert(mesh_patch.is_valid());
	Parameterizer::Error_code err = CGAL::parameterize(mesh_patch);
	assert(err == Parameterizer::OK);
	
	double mins[2], maxs[2];
	
	GX::Unwrapping::Mesh output;
	std::map<GX::Unwrapping::Mesh::Vertex, size_t> vertexMapping;
	size_t facetIndex = 0;
	for (auto facetIterator = mesh.facets_begin(); facetIterator not_eq mesh.facets_end(); facetIterator++)
	{
		assert(facetIterator->facet_degree() == 3);
		auto halfedgeIterator = facetIterator->facet_begin();
		for (size_t i = 0; i < 3; i++)
		{
			GX::Unwrapping::Mesh::Vertex outputVertex;
			auto vertex = halfedgeIterator->vertex();
			outputVertex.texcoord[0] = mesh_adaptor.info(& (*halfedgeIterator))->uv().x();
			outputVertex.texcoord[1] = mesh_adaptor.info(& (*halfedgeIterator))->uv().y();
			
			if (facetIndex == 0 and i == 0)
			{
				for (int j = 0; j < 2; j++)
				{
					maxs[j] = outputVertex.texcoord[j];
					mins[j] = outputVertex.texcoord[j];
				}
			}
			
			for (int j = 0; j < 2; j++)
			{
				maxs[j] = std::max(outputVertex.texcoord[j], maxs[j]);
				mins[j] = std::min(outputVertex.texcoord[j], mins[j]);
			}
			
			outputVertex.position[0] = vertex->point().x();
			outputVertex.position[1] = vertex->point().y();
			outputVertex.position[2] = vertex->point().z();
			
			auto qtangent = input.vertices[input.indices[facetIndex * 3 + i]].qtangent;
			for (size_t j = 0; j < 4; j++)
				outputVertex.qtangent[j] = qtangent[j];
			
			size_t index = 0;
			if (vertexMapping.find(outputVertex) == vertexMapping.end())
				vertexMapping[outputVertex] = vertexMapping.size();
			index = vertexMapping[outputVertex];
			output.indices.push_back(index);
			halfedgeIterator++;
		}
		facetIndex++;
	}
	
	output.vertices.reserve(vertexMapping.size());
	for (auto vertexIterator = vertexMapping.begin(); vertexIterator not_eq vertexMapping.end(); vertexIterator++)
	{
		GX::Unwrapping::Mesh::Vertex v = vertexIterator->first;
		for (int j = 0; j < 2; j++)
		{
			double difference = maxs[j] - mins[j];
			v.texcoord[j] -= mins[j];
			if (difference)
				v.texcoord[j] /= difference;
		}
		output.vertices[vertexIterator->second] = v;
	}
	
	return std::move(output);
}

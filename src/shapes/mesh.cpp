#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which
 * share an index and vertex buffer. Since individual triangles are rarely
 * needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of
     * the element corresponds to one vertex index (into @c m_vertices ) of the
     * triangle. This list will always contain as many elements as there are
     * triangles.
     */
    std::vector<Vector3i> m_triangles;
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be
     * fewer than @code 3 * numTriangles @endcode vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging
    /// purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the
    /// geometric normal instead.
    bool m_smoothNormals;

protected:
    int numberOfPrimitives() const override { return int(m_triangles.size()); }

    inline void populate(int primitiveIndex, SurfaceEvent &surf,
                         const Point &position, float u, float v) const {
        surf.position = position;

        const Vertex v1 = m_vertices[m_triangles[primitiveIndex][0]];
        const Vertex v2 = m_vertices[m_triangles[primitiveIndex][1]];
        const Vertex v3 = m_vertices[m_triangles[primitiveIndex][2]];
        surf.uv         = (v2.uv - v1.uv) * u + (v3.uv - v1.uv) * v;

        const Vector v1v = Vector(v1.position);
        const Vector v2v = Vector(v2.position);
        const Vector v3v = Vector(v3.position);

        surf.geometryNormal = (v2v - v1v).cross(v3v - v1v).normalized();

        surf.shadingNormal =
            m_smoothNormals ? Vertex::interpolate(Vector2(u, v), v1, v2, v3)
                                  .normal.normalized()
                            : surf.geometryNormal;

        surf.tangent = (v2v - v1v).normalized();

        surf.pdf = 0.0f;
    }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its,
                   Sampler &rng) const override {

        const Vector v1 =
            Vector(m_vertices[m_triangles[primitiveIndex][0]].position);
        const Vector v2 =
            Vector(m_vertices[m_triangles[primitiveIndex][1]].position);
        const Vector v3 =
            Vector(m_vertices[m_triangles[primitiveIndex][2]].position);

        const Vector c = Vector(ray.origin) - v1;

        Matrix3x3 m;

        m.setColumn(0, -ray.direction);
        m.setColumn(1, v2 - v1);
        m.setColumn(2, v3 - v1);

        Matrix3x3 mt = m;
        Matrix3x3 mu = m;
        Matrix3x3 mv = m;

        mt.setColumn(0, c);
        mu.setColumn(1, c);
        mv.setColumn(2, c);
        float detM = m.determinant();

        if (detM == 0.0f) {
            return false;
        }

        float t = mt.determinant() / detM;

        if (t < Epsilon || t > its.t) {
            return false;
        }

        float u = mu.determinant() / detM;
        float v = mv.determinant() / detM;

        if (u + v > 1.0f || u < 0.0f || v < 0.0f) {
            return false;
        }

        its.t = t;
        populate(primitiveIndex, its, ray(t), u, v);
        return true;

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the
        // triangle that should be intersected
        // * if m_smoothNormals is true, interpolate the vertex normals from
        // m_vertices
        //   * make sure that your shading frame stays orthonormal!
        // * if m_smoothNormals is false, use the geometrical normal (can be
        // computed from the vertex positions)
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        Vector v1 = Vector(m_vertices[m_triangles[primitiveIndex][0]].position);
        Vector v2 = Vector(m_vertices[m_triangles[primitiveIndex][1]].position);
        Vector v3 = Vector(m_vertices[m_triangles[primitiveIndex][2]].position);

        return Bounds(Point(std::min({ v1[0], v2[0], v3[0] }),
                            std::min({ v1[1], v2[1], v3[1] }),
                            std::min({ v1[2], v2[2], v3[2] })),
                      Point(std::max({ v1[0], v2[0], v3[0] }),
                            std::max({ v1[1], v2[1], v3[1] }),
                            std::max({ v1[2], v2[2], v3[2] })));
    }

    Point getCentroid(int primitiveIndex) const override {
        Vector v1 = Vector(m_vertices[m_triangles[primitiveIndex][0]].position);
        Vector v2 = Vector(m_vertices[m_triangles[primitiveIndex][1]].position);
        Vector v3 = Vector(m_vertices[m_triangles[primitiveIndex][2]].position);

        return (v1 + v2 + v3) / 3.0f;
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath  = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath, m_triangles, m_vertices);
        logger(EInfo,
               "loaded ply with %d triangles, %d vertices",
               m_triangles.size(),
               m_vertices.size());
        buildAccelerationStructure();
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        PROFILE("Triangle mesh")
        return AccelerationStructure::intersect(ray, its, rng);
    }

    AreaSample sampleArea(Sampler &rng) const override{
        // only implement this if you need triangle mesh area light sampling for
        // your rendering competition
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string());
    }
};

} // namespace lightwave

REGISTER_SHAPE(TriangleMesh, "mesh")

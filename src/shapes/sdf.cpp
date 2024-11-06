#include <lightwave.hpp>

namespace lightwave {
//This implementation of signed distance fields is NOT finished!
class Sdf : public Shape {
public:
    Sdf(const Properties &properties) {

    }

    inline float sdfFunction(const Point &position) const {
        //just a basic sdf for a spheres
        return Vector(position).length();
    }

    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;

        surf.uv.x() = (atan2f(position.z(), position.x()) + std::numbers::pi) /
                      (2 * std::numbers::pi);
        surf.uv.y() = (position.y() + 1) / 2;

        //TODO: calculate normals correctly
        surf.shadingNormal  = Vector(position);
        surf.geometryNormal = surf.shadingNormal;

        // Three different cases to account for all edge cases where the normal
        // in one direction is 0 0.5 is approximately 1/sqrt(3). More accuracy
        // is not needed here :)
        //TODO: calculate tangents correctly
        if (position.x() >= 0.5) {

            surf.tangent = Vector((-position.y() - position.z()) / position.x(),
                                  1.0f,
                                  1.0f)
                               .normalized();
        } else if (position.y() >= 0.5) {
            surf.tangent = Vector(1.0f,
                                  (-position.x() - position.z()) / position.y(),
                                  1.0f)
                               .normalized();
        } else {
            surf.tangent = Vector(1.0f,
                                  1.0f,
                                  (-position.x() - position.y()) / position.z())
                               .normalized();
        }
        surf.pdf = 0.0f;
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        float t = 0.0f;
        float dist = sdfFunction(ray.origin);
        Point position = ray.origin;

        //we expect that the ray reaches the bounding box at some point
        bool wasInsideBoundingBox = false;
        while (dist > Epsilon) {
            if (abs(position.x()) <= 1 && abs(position.y()) <= 1 && abs(position.z()) <= 1) {
                wasInsideBoundingBox = true;
            }
            if (wasInsideBoundingBox && dist > 1.0f) {
                //ray left the bounding box
                return false;
            }
            t += dist;
            position = ray(t);
            dist = sdfFunction(position);
        }

        if (t < Epsilon || t > its.t) {
            return false;
        }

        its.t = t;
        populate(its, position);

        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
    }

    Point getCentroid() const override { return Point(0); }

    AreaSample sampleArea(Sampler &rng) const override{ NOT_IMPLEMENTED }

    std::string toString() const override {
        return "Sphere[]";
    }
};
} // namespace lightwave

REGISTER_SHAPE(Sdf, "sdf")
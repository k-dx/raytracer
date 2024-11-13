#include <lightwave.hpp>

namespace lightwave {
class Sphere : public Shape {
public:
    Sphere(const Properties &properties) {}

    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;

        surf.uv.x() =
            atan2f(position.x(), position.z()) / (2.f * std::numbers::pi);
        surf.uv.y() = acos(position.y()) / std::numbers::pi;

        // normal should already be normalized.
        surf.shadingNormal  = Vector(position);
        surf.geometryNormal = surf.shadingNormal;

        // Three different cases to account for all edge cases where the normal
        // in one direction is 0 0.5 is approximately 1/sqrt(3). More accuracy
        // is not needed here :)
        if (abs(position.x()) >= 0.5) {
            surf.tangent = Vector((-position.y() - position.z()) / position.x(),
                                  1.0f,
                                  1.0f)
                               .normalized();
        } else if (abs(position.y()) >= 0.5) {
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
        const float od = Vector(ray.origin).dot(ray.direction);
        const float oo = Vector(ray.origin).dot(Vector(ray.origin));
        // since the ray direction is normalized, we have dd=1
        const float radicant = od * od - oo + 1;
        if (radicant <= 0.0f) {
            return false;
        }
        const float root = sqrt(radicant);

        const float t1 = -od + root;
        const float t2 = -od - root;
        const float t  = (t2 < t1 && t2 > Epsilon) || t1 <= Epsilon ? t2 : t1;
        if (t < Epsilon || t > its.t) {
            return false;
        }

        its.t = t;
        populate(its, ray(t));

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

REGISTER_SHAPE(Sphere, "sphere")
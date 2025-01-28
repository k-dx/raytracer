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

        // simplified Vector(position).cross(Vector(0.f, 0.f, 1.f))
        surf.tangent = Vector(position.y(), -position.x(), 0.f);

        surf.pdf = Inv4Pi;
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        const float od = Vector(ray.origin).dot(ray.direction);
        const float oo = Vector(ray.origin).dot(Vector(ray.origin));
        // since the ray direction is normalized, we have dd=1
        const float radicant = od * od - oo + 1.f;
        if (radicant < Epsilon) {
            return false;
        }
        const float root = sqrt(radicant);

        const float t1 = -od + root;
        const float t2 = -od - root;

        const float t = (t2 < t1 && t2 > Epsilon) || t1 <= Epsilon ? t2 : t1;
        if (t < Epsilon || t > its.t) {
            return false;
        }

        its.t          = t;
        Point position = ray(t);
        position.x()   = clamp(position.x(), -1.f, 1.f);
        position.y()   = clamp(position.y(), -1.f, 1.f);
        position.z()   = clamp(position.z(), -1.f, 1.f);
        populate(its, position);

        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
    }

    Point getCentroid() const override { return Point(0); }

    AreaSample sampleArea(Sampler &rng) const override {
        const Vector position = squareToUniformSphere(rng.next2D());
        AreaSample sample;
        populate(sample, position);
        return sample;
    }

    std::string toString() const override { return "Sphere[]"; }
};
} // namespace lightwave

REGISTER_SHAPE(Sphere, "sphere")
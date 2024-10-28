#include <lightwave.hpp>

namespace lightwave {
class Sphere : public Shape {
public:
    Sphere(const Properties &properties) {
        
    }
    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        float dd = ray.direction.dot(ray.direction);
        float od = Vector(ray.origin).dot(ray.direction);
        float oo = Vector(ray.origin).dot(Vector(ray.origin));
        return (od * od - 4 * dd * oo) > Epsilon;
    }
    Bounds getBoundingBox() const override {
        return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
    }
    Point getCentroid() const override {
        return Point(0);
    }
    AreaSample sampleArea(Sampler &rng) const override {
        NOT_IMPLEMENTED
    }
    std::string toString() const override { return "Sphere[]"; }
};
} // namespace lightwave

REGISTER_SHAPE(Sphere, "sphere")
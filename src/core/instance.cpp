#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf, const Vector &wo) const {
    Vector bitangentNotNormalized = surf.geometryNormal.cross(surf.tangent);
    bitangentNotNormalized = m_transform->apply(bitangentNotNormalized);

    surf.position = m_transform->apply(surf.position);
    Vector tangentNotNormalized = m_transform->apply(surf.tangent);
    surf.tangent  = tangentNotNormalized.normalized();
    surf.geometryNormal = m_transform->applyNormal(surf.geometryNormal).normalized();

    surf.shadingNormal =
        m_transform->applyNormal(surf.shadingNormal).normalized();

    if (m_normal) {
        const auto texture   = m_normal->evaluate(surf.uv);
        Vector shadingNormal = Vector(texture.r(), texture.g(), texture.b());

        if (shadingNormal.lengthSquared() == 0.0f) {
            logger(EWarn,
                   "Shading normal is of length 0, using value from "
                   "surface instead in instance %s",
                   toString());
        } else {
            // texture values can only be nonnegative, so we need to adjust the
            // normals from [0, 1]^3 to [-1, 1]^3
            shadingNormal = 2.0f * shadingNormal - Vector(1.0f);

            // "wrap" the normals around the object
            surf.shadingNormal =
                surf.shadingFrame().toWorld(shadingNormal).normalized();
        }
    }

    surf.pdf /= tangentNotNormalized.cross(bitangentNotNormalized).length();
}

inline void validateIntersection(const Intersection &its) {
    // use the following macros to make debugginer easier:
    // * assert_condition(condition, { ... });
    // * assert_normalized(vector, { ... });
    // * assert_ortoghonal(vec1, vec2, { ... });
    // * assert_finite(value or vector or color, { ... });

    // each assert statement takes a block of code to execute when it fails
    // (useful for printing out variables to narrow done what failed)

    assert_finite(its.t, {
        logger(
            EError,
            "  your intersection produced a non-finite intersection distance");
        logger(EError, "  offending shape: %s", its.instance->shape());
    });
    assert_condition(its.t >= Epsilon, {
        logger(EError,
               "  your intersection is susceptible to self-intersections");
        logger(EError, "  offending shape: %s", its.instance->shape());
        logger(EError,
               "  returned t: %.3g (smaller than Epsilon = %.3g)",
               its.t,
               Epsilon);
    });
}

bool Instance::intersect(const Ray &worldRay, Intersection &its,
                         Sampler &rng) const {
    const Intersection prevIts = its;
    if (!m_transform) {
        // fast path, if no transform is needed
        const Ray localRay  = worldRay;
        bool wasIntersected = m_shape->intersect(localRay, its, rng);
        if (wasIntersected) {
            validateIntersection(its);
            if (m_alpha && m_alpha->evaluate(its.uv).a() <= rng.next()) {
                its            = prevIts;
                wasIntersected = false;
            } else {
                its.instance = this;
            }
        }
        return wasIntersected;
    }

    Ray localRay    = m_transform->inverse(worldRay);
    float rayLength = localRay.direction.length();
    localRay        = localRay.normalized();

    // hints:
    // * transform the ray (do not forget to normalize!)
    // * how does its.t need to change?
    its.t *= rayLength;

    bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        validateIntersection(its);
    }
    if (!wasIntersected || (wasIntersected && m_alpha &&
                            m_alpha->evaluate(its.uv).a() <= rng.next())) {
        its            = prevIts;
        wasIntersected = false;
    }

    if (wasIntersected) {
        its.instance = this;
        validateIntersection(its);
        its.t /= rayLength;
        transformFrame(its, -localRay.direction);
    }

    return wasIntersected;
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);
    transformFrame(sample, Vector());
    return sample;
}

} // namespace lightwave

REGISTER_CLASS(Instance, "instance", "default")

#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // we don't need to abs(cosTheta(wi)) here, because it cancels out with
        // the cos from the rendering equation
        const float norm = 4.f * abs(Frame::cosTheta(wo));

        if (norm == 0.f) {
            return BsdfEval::invalid();
        }

        // hints:
        // the microfacet normal (half-vector) can be computed from `wi' and
        // `wo'
        const Vector wm = (wi + wo).normalized();

        const Color R    = m_reflectance->evaluate(uv);
        const float D    = microfacet::evaluateGGX(alpha, wm);
        const float G1_i = microfacet::smithG1(alpha, wm, wi);
        const float G1_o = microfacet::smithG1(alpha, wm, wo);

        return {
            .value = R * D * G1_i * G1_o / norm,
        };
    }

    virtual Color getAlbedo(const Point2 &uv) const override {
        return m_reflectance->evaluate(uv);
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        const Vector normal =
            microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());

        // we need to mirror the wo vector around the normal to get wi
        const float cos = wo.dot(normal);
        // the normal scaled so vector (scaled_normal - wo) is perpendicular to
        // normal
        const Vector scaled_normal = cos * normal;
        const Vector direction     = scaled_normal - wo;
        const Vector wi            = (scaled_normal + direction).normalized();
        // const Vector wi = (2.f * wo.dot(normal) * normal - wo).normalized();

        const Color R      = m_reflectance->evaluate(uv);
        const float G1_i   = microfacet::smithG1(alpha, normal, wi);
        const Color weight = R * G1_i;

        return {
            .wi     = wi,
            .weight = weight,
        };

        // hints:
        // * do not forget to cancel out as many terms from your equations as
        // possible!
        //   (the resulting sample weight is only a product of two factors)
    }

    std::string toString() const override {
        return tfm::format(
            "RoughConductor[\n"
            "  reflectance = %s,\n"
            "  roughness = %s\n"
            "]",
            indent(m_reflectance),
            indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")

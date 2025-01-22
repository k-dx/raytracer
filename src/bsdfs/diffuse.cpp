#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // we need to check if wi and wo are on the same hemisphere because we
        // don't want to go through objects
        const Color albedo    = Frame::sameHemisphere(wo, wi)
                                    ? m_albedo->evaluate(uv)
                                    : Color::black();
        const float cos_theta = Frame::absCosTheta(wi);
        return {
            .value = albedo * cos_theta / std::numbers::pi,
        };
    }

    virtual Color getAlbedo(const Point2 &uv) const override {
        return m_albedo->evaluate(uv);
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const Vector wi = squareToCosineHemisphere(rng.next2D());
        return {
            .wi     = Frame::sameHemisphere(wi, wo) ? wi : -wi,
            .weight = m_albedo->evaluate(uv),
        };
    }

    std::string toString() const override {
        return tfm::format(
            "Diffuse[\n"
            "  albedo = %s\n"
            "]",
            indent(m_albedo));
    }
};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")

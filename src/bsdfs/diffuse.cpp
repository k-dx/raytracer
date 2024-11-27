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
        const Color albedo = m_albedo->evaluate(uv);
        return {
            .value = albedo / std::numbers::pi,
        };
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // const Vector wi = squareToUniformHemisphere(rng.next2D());
        const Vector wi = squareToCosineHemisphere(rng.next2D());

        const float cos_theta = wi[2];
        Color weight          = Color::white();
        if (cosineHemispherePdf(wi) != 0.f) {
            weight = evaluate(uv, wo, wi).value * cos_theta /
                     cosineHemispherePdf(wi);
            // uniformHemispherePdf();
        }

        assert_condition(wi.length() < 1.f + Epsilon &&
                             wi.length() > 1.f - Epsilon, );
        assert_condition(!std::isnan(wi[0]), );
        assert_condition(!std::isnan(wi[1]), );
        assert_condition(!std::isnan(wi[2]), );

        assert_condition(!std::isnan(wi.length()), );

        return {
            .wi     = wi,
            .weight = weight,
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

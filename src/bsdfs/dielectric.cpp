#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        float ior         = m_ior->scalar(uv);
        float inv_ior     = 1.f / ior;
        float cos_theta_o = wo[2];

        float radicant =
            1.f - inv_ior * inv_ior * (1.f - cos_theta_o * cos_theta_o);
        if (radicant >= Epsilon) {
            float cos_theta_i = sqrtf(radicant);
            Vector wi_refract =
                -inv_ior * wo +
                Vector(0, 0, -inv_ior * cos_theta_o + cos_theta_i);

            const float rp = (inv_ior * cos_theta_i - cos_theta_o) /
                             (inv_ior * cos_theta_i + cos_theta_o);
            const float rs = (cos_theta_i - inv_ior * cos_theta_o) /
                             (cos_theta_i + inv_ior * cos_theta_o);
            const float fresnel = (rp * rp + rs * rs) / 2.f;

            if (rng.next() > fresnel) {
                const Color transmittance =
                    m_transmittance->evaluate(uv) * inv_ior * inv_ior;
                return {
                    .wi     = wi_refract.normalized(),
                    .weight = transmittance,
                };
            }
        }

        Vector wi_reflect       = Vector(-wo[0], -wo[1], wo[2]);
        const Color reflectance = m_reflectance->evaluate(uv);
        return {
            .wi     = wi_reflect,
            .weight = reflectance,
        };
    }

    std::string toString() const override {
        return tfm::format(
            "Dielectric[\n"
            "  ior           = %s,\n"
            "  reflectance   = %s,\n"
            "  transmittance = %s\n"
            "]",
            indent(m_ior),
            indent(m_reflectance),
            indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")

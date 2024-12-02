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
        float oir         = 1.f / ior;
        float cos_theta_o = wo[2];
        // if (cos_theta_o > 0.f) {
        //     std::swap(ior, oir);
        // }
        /*
        float r;
        if (cos_theta_o > 0.f) {
            r = 1.f / ior;
        } else {
            cos_theta_o = -cos_theta_o;
            r           = ior;
        }
        */
        if (cos_theta_o > 0.f) {
            float radicant =
                1.f - oir * oir * (1.f - cos_theta_o * cos_theta_o);
            if (radicant >= Epsilon) {
                float cos_theta_i = sqrtf(radicant);
                // float sin_theta_i = sqrt(1.f - radicant);

                // Vector direction = Vector(-wo[0], -wo[1], 0).normalized();
                // Vector wi_refract =
                //     (direction * sin_theta_i / cos_theta_i + Vector(0, 0,
                //     -1))
                //         .normalized();
                //  float cos_theta_i = sqrtf(radicant);

                Vector wi_refract =
                    -oir * wo +
                    Vector(0, 0, -(-oir * cos_theta_o + cos_theta_i));

                const float rp = (oir * cos_theta_i - cos_theta_o) /
                                 (oir * cos_theta_i + cos_theta_o);
                const float rs = (cos_theta_i - oir * cos_theta_o) /
                                 (cos_theta_i + oir * cos_theta_o);
                const float fresnel = 0.0; //(rp * rp + rs * rs) / 2.f;

                if (rng.next() > fresnel) {
                    const Color transmittance =
                        m_transmittance->evaluate(uv) * oir * oir;
                    return {
                        .wi     = wi_refract.normalized(),
                        .weight = transmittance,
                    };
                }
            }
        } else {
            float radicant =
                1.f - ior * ior * (1.f - cos_theta_o * cos_theta_o);
            if (radicant >= Epsilon) {
                float cos_theta_i = sqrtf(radicant);

                Vector wi_refract =
                    -ior * wo + Vector(0, 0, ior * cos_theta_o + cos_theta_i);

                const float fresnel = 0.0; //(rp * rp + rs * rs) / 2.f;

                if (rng.next() > fresnel) {
                    const Color transmittance =
                        m_transmittance->evaluate(uv) * ior * ior;
                    return {
                        .wi     = wi_refract.normalized(),
                        .weight = transmittance,
                    };
                }
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

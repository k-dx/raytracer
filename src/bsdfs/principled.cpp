#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        // we need to check if wi and wo are on the same hemisphere because we
        // don't want to go through objects
        const Color albedo =
            Frame::sameHemisphere(wo, wi) ? color : Color::black();
        const float cos_theta = Frame::absCosTheta(wi);
        return {
            .value = albedo * cos_theta / std::numbers::pi,
        };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        assert_normalized(wo, {});
        const Vector wi = squareToCosineHemisphere(rng.next2D());
        return {
            .wi     = Frame::sameHemisphere(wi, wo) ? wi : -wi,
            .weight = color,
        };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
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

        const Color R    = color;
        const float D    = microfacet::evaluateGGX(alpha, wm);
        const float G1_i = microfacet::smithG1(alpha, wm, wi);
        const float G1_o = microfacet::smithG1(alpha, wm, wo);

        return {
            .value = R * D * G1_i * G1_o / norm,
        };

        // hints:
        // * copy your roughconductor bsdf evaluate here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
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

        const Color R      = color;
        const float G1_i   = microfacet::smithG1(alpha, normal, wi);
        const Color weight = R * G1_i;

        return {
            .wi     = wi,
            .weight = weight,
        };

        // hints:
        // * copy your roughconductor bsdf sample here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = m_specular->scalar(uv);
        const auto metallic = m_metallic->scalar(uv);
        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        const auto diffuseAlbedo = diffuseLobe.color.mean();
        const auto totalAlbedo =
            diffuseLobe.color.mean() + metallicLobe.color.mean();
        return {
            .diffuseSelectionProb =
                totalAlbedo > 0 ? diffuseAlbedo / totalAlbedo : 1.0f,
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);

        const auto diffuse  = combination.diffuse.evaluate(wo, wi);
        const auto metallic = combination.metallic.evaluate(wo, wi);

        return {
            .value = diffuse.value + metallic.value,
        };

        // hint: evaluate `combination.diffuse` and `combination.metallic` and
        // combine their results
    }

    virtual Color getAlbedo(const Point2 &uv) const override {
        return m_baseColor->evaluate(uv);
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);

        BsdfSample sample;
        float samplingProbability;

        if (rng.next() <= combination.diffuseSelectionProb) {
            sample              = combination.diffuse.sample(wo, rng);
            samplingProbability = combination.diffuseSelectionProb;
        } else {
            sample              = combination.metallic.sample(wo, rng);
            samplingProbability = 1.f - combination.diffuseSelectionProb;
        }

        if (sample.isInvalid() || samplingProbability <= 0.f) {
            return BsdfSample::invalid();
        }

        return {
            .wi     = sample.wi,
            .weight = sample.weight / samplingProbability,
        };

        // hint: sample either `combination.diffuse` (probability
        // `combination.diffuseSelectionProb`) or `combination.metallic`
    }

    std::string toString() const override {
        return tfm::format(
            "Principled[\n"
            "  baseColor = %s,\n"
            "  roughness = %s,\n"
            "  metallic  = %s,\n"
            "  specular  = %s,\n"
            "]",
            indent(m_baseColor),
            indent(m_roughness),
            indent(m_metallic),
            indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")

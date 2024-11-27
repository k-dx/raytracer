#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {
public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        // Scene intersection
        const Intersection its = m_scene->intersect(ray, rng);
        if (!its) {
            return its.evaluateEmission().value;
        }

        // Delta lights
        const LightSample lightSample = m_scene->sampleLight(rng);
        const Light *light            = lightSample.light;
        Color lightContribution       = Color::black();

        if (light) {
            const DirectLightSample directLight =
                light->sampleDirect(its.position, rng);

            const Ray lightRay(its.position, directLight.wi);

            const Intersection lightOcclusionIts =
                m_scene->intersect(lightRay, rng);
            if (directLight.distance >= Epsilon &&
                (!lightOcclusionIts ||
                 lightOcclusionIts.t - Epsilon >= directLight.distance)) {

                const Color fr = its.evaluateBsdf(directLight.wi).value;

                const float cos_theta =
                    abs(its.shadingNormal.dot(directLight.wi));

                lightContribution = directLight.weight * cos_theta * fr /
                                    lightSample.probability;
            }
        }

        // Emissive shapes
        Color emissionContribution = Color(0);
        const BsdfSample emission  = its.sampleBsdf(rng);
        if (emission) {
            const Ray emissionRay(its.position, emission.wi);
            const Intersection emissionIts =
                m_scene->intersect(emissionRay, rng);

            EmissionEval emissionEval = emissionIts.evaluateEmission();

            assert_condition(!std::isnan(emissionEval.value[0]), );
            assert_condition(!std::isnan(emission.weight[0]), );

            emissionContribution = emissionEval.value * emission.weight;
        }

        return lightContribution + emissionContribution +
               its.evaluateEmission().value;
    }

    std::string toString() const override {
        return tfm::format(
            "DirectIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(DirectIntegrator, "direct")

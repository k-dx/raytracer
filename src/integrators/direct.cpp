#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {
public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        // logger(EInfo, "Entered Li");
        // Scene intersection
        const Intersection its = m_scene->intersect(ray, rng);
        if (!its) {
            return its.evaluateEmission().value;
        }

        // Delta lights
        const LightSample lightSample = m_scene->sampleLight(rng);
        const Light *light            = lightSample.light;
        const DirectLightSample directLight =
            light->sampleDirect(its.position, rng);

        const Ray lightRay(its.position, directLight.wi);

        Color lightContribution;
        const Intersection lightIts = m_scene->intersect(lightRay, rng);
        if (directLight.distance < Epsilon ||
            (lightIts && lightIts.t - Epsilon < directLight.distance)) {
            // light is occluded
            lightContribution = Color::black();
        } else {
            const Color fr = its.evaluateBsdf(directLight.wi).value;

            const float cos_theta = abs(its.shadingNormal.dot(directLight.wi));

            lightContribution =
                directLight.weight * cos_theta * fr / lightSample.probability;
        }

        // Emissive shapes
        // const BsdfSample emission = its.sampleBsdf(rng);
        // const Ray emissionRay(its.position, emission.wi);
        // const Intersection emissionIts = m_scene->intersect(emissionRay,
        // rng);

        Color emissionContribution = Color(0);
        // if (emissionIts) {
        //     if (emissionIts.lightProbability < Epsilon) {
        //         emissionContribution = Color(0);
        //     } else {
        //         const float cos_theta =
        //         abs(its.shadingNormal.dot(emission.wi));

        //         emissionContribution =
        //             emissionIts.evaluateBsdf(-emission.wi).value /
        //             emissionIts.lightProbability * emission.weight *
        //             cos_theta;
        //     }
        // } else {
        //     emissionContribution =
        //         emissionIts.evaluateEmission().value * emission.weight;
        // }

        // logger(EInfo, "Im still alive");

        return lightContribution + emissionContribution;
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

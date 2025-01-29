#include <lightwave.hpp>

namespace lightwave {

class PathtracerIntegrator : public SamplingIntegrator {
    int m_depth;

public:
    PathtracerIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_depth = properties.get<int>("depth", 2);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Color weight   = Color::white();
        Color emission = Color::black();
        Ray currentRay = ray;

        Intersection its = m_scene->intersect(currentRay, rng);
        emission += its.evaluateEmission().value;

        for (int i = 0; its && i < m_depth - 1; i++) {
            // Delta lights
            const LightSample lightSample = m_scene->sampleLight(rng);
            const Light *light            = lightSample.light;

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

                    emission += directLight.weight * weight * fr /
                                lightSample.probability;
                }
            }

            const BsdfSample sample = its.sampleBsdf(rng);
            if (!sample) {
                break;
            }

            weight *= sample.weight;

            currentRay = Ray(its.position, sample.wi, i + 1);

            its = m_scene->intersect(currentRay, rng);
            emission += its.evaluateEmission().value * weight;
        }

        return emission;
    }

    std::string toString() const override {
        return tfm::format(
            "PathtracerIntegrator[\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(PathtracerIntegrator, "pathtracer")

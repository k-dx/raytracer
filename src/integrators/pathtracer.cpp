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

        for (int i = 0; i < m_depth; i++) {
            // Scene intersection
            const Intersection its = m_scene->intersect(currentRay, rng);

            emission += its.evaluateEmission().value * weight;

            if (!its) {
                break;
            }

            // Delta lights
            const LightSample lightSample = m_scene->sampleLight(rng);
            const Light *light            = lightSample.light;

            if (light) {
                const DirectLightSample directLight =
                    light->sampleDirect(its.position, rng);

                // check if sampled light is on the same side as the ray
                if (std::signbit(its.shadingNormal.dot(its.wo)) ==
                    std::signbit(its.shadingNormal.dot(directLight.wi))) {

                    const Ray lightRay(its.position, directLight.wi);

                    const Intersection lightOcclusionIts =
                        m_scene->intersect(lightRay, rng);
                    if (directLight.distance >= Epsilon &&
                        (!lightOcclusionIts || lightOcclusionIts.t - Epsilon >=
                                                   directLight.distance)) {

                        const Color fr = its.evaluateBsdf(directLight.wi).value;

                        const float cos_theta =
                            abs(its.shadingNormal.dot(directLight.wi));

                        emission += directLight.weight * cos_theta * weight *
                                    fr / lightSample.probability;
                    }
                }
            }

            const BsdfSample sample = its.sampleBsdf(rng);
            if (!sample) {
                break;
            }
            assert_condition(weight.mean() - 1.f < Epsilon, {});
            weight *= sample.weight;
            currentRay = Ray(its.position, sample.wi, i);
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

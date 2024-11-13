#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {
public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        if (!its) {
            return its.evaluateEmission().value;
        }

        const Light *light            = m_scene->sampleLight(rng).light;
        DirectLightSample directLight = light->sampleDirect(its.position, rng);

        const Ray lightRay(its.position, directLight.wi);

        Intersection lightIts = m_scene->intersect(lightRay, rng);
        if (directLight.distance < Epsilon ||
            (lightIts && lightIts.t - Epsilon < directLight.distance)) {
            return Color::black();
        }

        Color fr        = its.evaluateBsdf(directLight.wi).value;
        float cos_theta = abs(its.shadingNormal.dot(directLight.wi));

        return directLight.weight * fr * cos_theta;
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

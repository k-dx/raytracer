#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
private:
    ref<Instance> m_instance;

public:
    AreaLight(const Properties &properties) : Light(properties) {
        m_instance = properties.getChild<Instance>();
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        const AreaSample sample = m_instance->sampleArea(rng);
        const Vector wi         = sample.position - origin;
        const Vector wiLocal = sample.shadingFrame().toLocal(-wi).normalized();
        const EmissionEval emission =
            m_instance->emission()->evaluate(sample.uv, wiLocal);
        const float distance = wi.length();
        return {
            .wi       = wi.normalized(),
            .weight   = emission.value / (sample.pdf * sqr(distance)),
            .distance = distance,
        };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "AreaLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")

#include <lightwave.hpp>
#include <limits>

namespace lightwave {

class DirectionalLight final : public Light {
private:
    Vector m_direction;
    Color m_intensity;

public:
    DirectionalLight(const Properties &properties) : Light(properties) {
        m_direction = properties.get<Vector>("direction").normalized();
        m_intensity = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Vector wi      = m_direction;
        float distance = std::numeric_limits<float>::infinity();
        return {
            .wi       = wi,
            .weight   = m_intensity,
            .distance = distance,
        };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "DirectionalLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")

#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {
    Color m_color0;
    Color m_color1;
    Vector2 m_scale;

public:
    CheckerboardTexture(const Properties &properties) {
        m_color0 = properties.get<Color>("color0", Color::black());
        m_color1 = properties.get<Color>("color1", Color::white());
        m_scale  = properties.get<Vector2>("scale");
    }

    Color evaluate(const Point2 &uv) const override {
        bool checkerboardU = (int) (uv[0] * m_scale[0]) % 2;
        bool checkerboardV = (int) (uv[1] * m_scale[1]) % 2;
        return checkerboardU == checkerboardV ? m_color0 : m_color1;
    }

    std::string toString() const override {
        return tfm::format(
            "CheckerboardTexture[\n"
            "  color0 = %s\n"
            "  color1 = %s\n"
            "  scale  = %s\n"
            "]",
            indent(m_color0),
            indent(m_color1),
            indent(m_scale));
    }
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")

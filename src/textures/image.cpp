#include <lightwave.hpp>

namespace lightwave {

class ImageTexture : public Texture {
    enum class BorderMode {
        Clamp,
        Repeat,
    };

    enum class FilterMode {
        Nearest,
        Bilinear,
    };

    ref<Image> m_image;
    float m_exposure;
    BorderMode m_border;
    FilterMode m_filter;

public:
    ImageTexture(const Properties &properties) {
        if (properties.has("filename")) {
            m_image = std::make_shared<Image>(properties);
        } else {
            m_image = properties.getChild<Image>();
        }
        m_exposure = properties.get<float>("exposure", 1);

        // clang-format off
        m_border = properties.getEnum<BorderMode>("border", BorderMode::Repeat, {
            { "clamp", BorderMode::Clamp },
            { "repeat", BorderMode::Repeat },
        });

        m_filter = properties.getEnum<FilterMode>("filter", FilterMode::Bilinear, {
            { "nearest", FilterMode::Nearest },
            { "bilinear", FilterMode::Bilinear },
        });
        // clang-format on
    }

    inline Color getColorAt(const Point2i &uv) const {
        int u;
        int v;

        switch (m_border) {
            default:
            case BorderMode::Clamp: {
                u = clamp(uv[0], 0, m_image->resolution()[0]);
                v = clamp(uv[1], 0, m_image->resolution()[1]);
                break;
            }
            case BorderMode::Repeat: {
                const int rx = m_image->resolution()[0];
                const int ry = m_image->resolution()[1];
                u = uv[0] % rx;
                v = uv[1] % ry;
                if (u < 0) {
                    u += rx;
                }
                if (v < 0) {
                    v += ry;
                }
                break;
            }
        }

        return m_image->get(Point2i(u, m_image->resolution()[1] - v - 1));// * m_exposure;
    }

    Color evaluate(const Point2 &uv) const override{
        const float u = uv[0] * m_image->resolution()[0];
        const float v = uv[1] * m_image->resolution()[1];

        float tmp;
        const float blendX = u >= 0 ? modff(u, &tmp) : modff(u, &tmp) + 1.0f;
        const float blendY = v >= 0 ? modff(v, &tmp) : modff(v, &tmp) + 1.0f;

        const int ui = u >= 0 ? (int) u : (int) u - 1;
        const int vi = v >= 0 ? (int) v : (int) v - 1;

        switch (m_filter) {
            default:
            case FilterMode::Nearest: {
                const int ur = blendX < 0.5f ? ui : ui + 1;
                const int vr = blendY < 0.5f ? vi : vi + 1;
                //return getColorAt(Point2i(ur, vr));
                return Color::white();
            }
            case FilterMode::Bilinear: {
                const Color c11 = getColorAt(Point2i(ui, vi));
                const Color c12 = getColorAt(Point2i(ui + 1, vi));
                const Color c21 = getColorAt(Point2i(ui, vi + 1));
                const Color c22 = getColorAt(Point2i(ui + 1, vi + 1));

                const Color c1 = c11 * (1.f - blendX) + c12 * blendX;
                const Color c2 = c21 * (1.f - blendX) + c22 * blendX;

                //return c1 * (1.f - blendY) + c2 * blendY;
                return Color::white();
            }
        }
    }

    std::string toString() const override {
        return tfm::format(
            "ImageTexture[\n"
            "  image = %s,\n"
            "  exposure = %f,\n"
            "]",
            indent(m_image),
            m_exposure);
    }
};

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")

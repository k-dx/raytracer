#include <lightwave.hpp>

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 *
 * In local coordinates (before applying m_transform), the camera looks in
 * positive z direction [0,0,1]. Pixels on the left side of the image ( @code
 * normalized.x < 0 @endcode ) are directed in negative x direction ( @code
 * ray.direction.x < 0 ), and pixels at the bottom of the image ( @code
 * normalized.y < 0 @endcode ) are directed in negative y direction ( @code
 * ray.direction.y < 0 ).
 */
class Perspective : public Camera {
private:
    float imageAspectRatio;
    float fovTan;
    char fovAxis;

public:
    Perspective(const Properties &properties) : Camera(properties) {
        logger(EDebug, "Constructor of Perspective");

        const float fov = properties.get<float>("fov");
        fovAxis         = properties.get<std::string>("fovAxis")[0];

        logger(EDebug, "fov: %d", fov);
        fovTan = tan(fov / 2.f * Pi / 180.f);
        logger(EDebug, "fovTan: %f", fovTan);

        logger(EDebug, "resolution: %s", m_resolution);
        imageAspectRatio = (float) m_resolution[0] / (float) m_resolution[1];

        logger(EDebug, "aspect ratio: %d", imageAspectRatio);

        // hints:
        // * precompute any expensive operations here (most importantly
        // trigonometric functions)
        // * use m_resolution to find the aspect ratio of the image
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        float rayX = normalized.x() * fovTan;
        if (fovAxis == 'y') {
            rayX *= imageAspectRatio;
        }
        float rayY = normalized.y() * fovTan;
        if (fovAxis == 'x') {
            rayY /= imageAspectRatio;
        }

        const auto direction = Vector(rayX, rayY, 1.f).normalized();

        return CameraSample{ .ray = m_transform->apply(
                                 Ray(Vector(0.f, 0.f, 0.f), direction)),
                             .weight = Color(1.0f) };

        // hints:
        // * use m_transform to transform the local camera coordinate system
        // into the world coordinate system
    }

    std::string toString() const override {
        return tfm::format(
            "Perspective[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  transform = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            indent(m_transform));
    }
};

} // namespace lightwave

REGISTER_CAMERA(Perspective, "perspective")

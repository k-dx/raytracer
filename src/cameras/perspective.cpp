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
    float factorX;
    float factorY;

public:
    Perspective(const Properties &properties) : Camera(properties) {
        const float fov    = properties.get<float>("fov");
        const char fovAxis = properties.get<std::string>("fovAxis")[0];

        const float fovTan = tan(fov / 2.f * Pi / 180.f);
        const float imageAspectRatio = (float) m_resolution[0] / (float) m_resolution[1];

        factorX = fovTan;
        if (fovAxis == 'y') {
            factorX *= imageAspectRatio;
        }
        factorY = fovTan;
        if (fovAxis == 'x') {
            factorY /= imageAspectRatio;
        }
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        float rayX = normalized.x() * factorX;
        float rayY = normalized.y() * factorY;

        const auto direction = Vector(rayX, rayY, 1.f);

        return CameraSample{ .ray = m_transform->apply(
                                 Ray(Vector(0.f, 0.f, 0.f), direction)).normalized(),
                             .weight = Color(1.0f) };
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

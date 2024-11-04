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
class Thinlens : public Camera {
private:
    float factorX;
    float factorY;
    float focusDistance;
    float apertureDiameter;

public:
    Thinlens(const Properties &properties) : Camera(properties) {
        const float fov           = properties.get<float>("fov");
        const char fovAxis        = properties.get<std::string>("fovAxis")[0];
        focusDistance             = properties.get<float>("focusDistance");
        const float aperture      = properties.get<float>("aperture");
        //if this parameter is present treat "aperture" as the f-number of the aperture and focalLength as the focal length f
        //otherwise treaet aperture as the diameter of the aperture
        const float focalLength   = properties.get<float>("focalLength", aperture * aperture);

        apertureDiameter = focalLength / aperture;

        const float fovTan = tan(fov / 2.f * Pi / 180.f);
        const float scaleFactor = fovTan * focusDistance;
        const float imageAspectRatio = (float) m_resolution[0] / (float) m_resolution[1];

        factorX = scaleFactor;
        if (fovAxis == 'y') {
            factorX *= imageAspectRatio;
        }
        factorY = scaleFactor;
        if (fovAxis == 'x') {
            factorY /= imageAspectRatio;
        }
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        //square root to distribute the rays equally over the aperture
        const float radius = sqrt(rng.next()) * apertureDiameter;
        const float angle = rng.next() * 2 * Pi;
        const Vector source = Vector(radius * sin(angle), radius * cos(angle), 0.0);

        const Vector target = Vector(normalized.x() * factorX, normalized.y() * factorY, focusDistance);

        const auto direction = (target - source).normalized();

        return CameraSample{ .ray = m_transform->apply(
                                 Ray(source, direction)),
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

REGISTER_CAMERA(Thinlens, "thinlens")

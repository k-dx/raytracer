#include <lightwave/core.hpp>
#include <lightwave/image.hpp>
#include <lightwave/registry.hpp>

#include <stb_image.h>
#include <tinyexr.h>

namespace lightwave {

void Image::loadImage(const std::filesystem::path &path, bool isLinearSpace) {
    const auto extension = path.extension();
    logger(EInfo, "loading image %s", path);
    if (extension == ".exr") {
        // loading of EXR files is handled by TinyEXR
        float *data;
        const char *err;
        if (LoadEXR(&data,
                    &m_resolution.x(),
                    &m_resolution.y(),
                    path.generic_string().c_str(),
                    &err)) {
            lightwave_throw("could not load image %s: %s", path, err);
        }

        m_data.resize(m_resolution.x() * m_resolution.y());
        auto it = data;
        for (auto &pixel : m_data) {
            for (int i = 0; i < pixel.NumComponents; i++)
                pixel[i] = *it++;
        }
        free(data);
    } else {
        // anything that is not an EXR file is handled by stb
        stbi_ldr_to_hdr_gamma(isLinearSpace ? 1.0f : 2.2f);

        int numChannels;
        float *data = stbi_loadf(path.generic_string().c_str(),
                                 &m_resolution.x(),
                                 &m_resolution.y(),
                                 &numChannels,
                                 4);
        if (data == nullptr) {
            lightwave_throw(
                "could not load image %s: %s", path, stbi_failure_reason());
        }

        m_data.resize(m_resolution.x() * m_resolution.y());
        auto it = data;
        for (auto &pixel : m_data) {
            for (int i = 0; i < pixel.NumComponents; i++)
                pixel[i] = *it++;
        }
        free(data);
    }
}

void Image::saveAt(const std::filesystem::path &path) const {
    const char *error;

    if (resolution().isZero()) {
        logger(EWarn, "cannot save empty image %s!", path);
        return;
    }

    logger(EInfo, "saving image %s", path);

    std::vector<Color> data;
    for (auto pixel : m_data) {
        data.push_back(pixel);
        data.back().a() = 1.f;
    }
    if (SaveEXR(reinterpret_cast<const float *>(data.data()),
                m_resolution.x(),
                m_resolution.y(),
                Color::NumComponents,
                true,
                path.generic_string().c_str(),
                &error)) {
        logger(EError, "  error saving image %s: %s", path, error);
    }
}
} // namespace lightwave

REGISTER_CLASS(Image, "image", "default")

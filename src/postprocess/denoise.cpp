#include <lightwave.hpp>
#include <OpenImageDenoise/oidn.hpp>

namespace lightwave {

class Denoise : public Postprocess {
protected:
    ref<Image> m_normal;
    ref<Image> m_albedo;
public:
    Denoise(const Properties &properties): Postprocess(properties) {
        m_normal = properties.getOptional<Image>("normals");
        m_albedo = properties.getOptional<Image>("albedo");
    }

    void fillBuffer(oidn::BufferRef buffer, ref<Image> image) {
        float* colorPtr = (float*)buffer.getData();
        for (int i = 0; i < image->resolution().y(); i++) {
            for (int j = 0; j < image->resolution().x(); j++) {
                Color color = image->get(Point2i(j, i));
                *colorPtr = color.r();
                colorPtr++;
                *colorPtr = color.g();
                colorPtr++;
                *colorPtr = color.b();
                colorPtr++;
            }
        }
    }

    void readBuffer(oidn::BufferRef buffer, ref<Image> image) {
        float* colorPtr = (float*)buffer.getData();
        for (int i = 0; i < image->resolution().y(); i++) {
            for (int j = 0; j < image->resolution().x(); j++) {
                Color color;
                color.r() = *colorPtr;
                colorPtr++;
                color.g() = *colorPtr;
                colorPtr++;
                color.b() = *colorPtr;
                colorPtr++;
                color.a() = 1;
                image->get(Point2i(j, i)) = color;
            }
        }
    }

    void execute() override {
        Point2i resolution = m_input->resolution();

        m_output->initialize(resolution);
        Streaming streaming(*m_output.get());

        // Create an Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();

        oidn::FilterRef filter = device.newFilter("RT");

        oidn::BufferRef colorBuf  = device.newBuffer(resolution.x() * resolution.y() * 3 * sizeof(float));
        filter.setImage("color",  colorBuf,  oidn::Format::Float3, resolution.x(), resolution.y());
        fillBuffer(colorBuf, m_input);

        if (m_normal) {
            oidn::BufferRef normalBuf  = device.newBuffer(resolution.x() * resolution.y() * 3 * sizeof(float));
            filter.setImage("normal",  normalBuf,  oidn::Format::Float3, resolution.x(), resolution.y());
            fillBuffer(normalBuf, m_normal);
        }

        if (m_albedo) {
            oidn::BufferRef albedoBuf  = device.newBuffer(resolution.x() * resolution.y() * 3 * sizeof(float));
            filter.setImage("albedo",  albedoBuf,  oidn::Format::Float3, resolution.x(), resolution.y());
            fillBuffer(albedoBuf, m_albedo);
        }

        filter.setImage("output", colorBuf,  oidn::Format::Float3, resolution.x(), resolution.y());
        filter.set("hdr", true); // beauty image is HDR
        filter.commit();

        // Filter the beauty image
        filter.execute();

        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
            logger(EError, errorMessage);

        // Get output image
        readBuffer(colorBuf, m_output);

        streaming.update();
        m_output->save();
    }

    std::string toString() const override {
        return "Denoise";
    }
};

} // namespace lightwave

REGISTER_POSTPROCESS(Denoise, "denoise")

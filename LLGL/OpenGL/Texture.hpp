#pragma once

#include <EssaUtil/Color.hpp>
#include <EssaUtil/Rect.hpp>
#include <string>
#include <utility>

namespace llgl::opengl {

class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture& other) = delete;
    Texture& operator=(const Texture& other) = delete;

    Texture(Texture&& other)
    {
        *this = std::move(other);
    }

    Texture& operator=(Texture&& other);

    // TODO: createFromImage
    // TODO: Add some Image/Bitmap class
    enum class Format {
        RGBA,
        RGB
    };

    static Texture create_from_color_array(Util::Vector2u size, Util::Colorf const* array, Format);
    static Texture create_empty(Util::Vector2u size, Format = Format::RGBA);
    static Texture create_from_id(int);

    void update(Util::Vector2u dst_pos, Util::Vector2u src_size, Util::Colorf const* array, Format);
    void recreate(Util::Vector2u, Format);

    unsigned id() const { return m_id; }
    Util::Vector2u size() const { return m_size; }

    void set_label(std::string const&);

    static void bind(Texture const* texture);

private:
    unsigned m_id { 0 };
    Util::Vector2u m_size;
    bool m_initialized = false;
};

class TextureBinder {
public:
    TextureBinder(Texture const& texture) { Texture::bind(&texture); }
    ~TextureBinder() { Texture::bind(nullptr); }
};

}

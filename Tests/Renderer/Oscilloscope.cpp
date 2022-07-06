#include "LLGL/Renderer/RenderToTexture.hpp"
#include "LLGL/Window/Mouse.hpp"
#include <LLGL/Core/Color.hpp>
#include <LLGL/Core/DelayedInit.hpp>
#include <LLGL/Core/VectorUtils.hpp>
#include <LLGL/OpenGL/Error.hpp>
#include <LLGL/OpenGL/FBO.hpp>
#include <LLGL/OpenGL/Shader.hpp>
#include <LLGL/OpenGL/Shaders/Basic330Core.hpp>
#include <LLGL/OpenGL/Utils.hpp>
#include <LLGL/OpenGL/Vertex.hpp>
#include <LLGL/Renderer/Transform.hpp>
#include <LLGL/Window/Window.hpp>
#include <SDL2/SDL_video.h>
#include <iostream>

static llgl::opengl::Program& shader()
{
    static llgl::DelayedInit<llgl::opengl::Program> program;
    if (!program.is_initialized()) {
        static char const* VERTEX_SHADER = R"~~~(
#version 410 core

layout(location=1) in vec4 position;

out vec4 f_Position;

void main()
{
    f_Position = position;
    gl_Position = position;
}
)~~~";
        static char const* FRAGMENT_SHADER = R"~~~(
#version 410 core

uniform sampler2D accum;
uniform sampler2D pass1;
uniform vec2 fbSize;

in vec4 f_Position;

void main()
{
    vec2 fragPos = f_Position.st / 2 + vec2(0.5, 0.5);
    vec4 color;

    // Box blur
    for (int x = -3; x <= 3; x++) {
        for (int y = -3; y <= 3; y++) {
            color += texture2D(pass1, fragPos + vec2(x/fbSize.x, y/fbSize.y)) / 49;
        }
    }

    // Motion blur
    vec4 accumFactor = texture2D(accum, fragPos) * 0.9;
    if (accumFactor.r < 0.05)
        accumFactor.r = 0;
    if (accumFactor.g < 0.05)
        accumFactor.g = 0;
    if (accumFactor.b < 0.05)
        accumFactor.b = 0;
    gl_FragColor = color + accumFactor;
}
)~~~";
        auto objects = {
            llgl::opengl::ShaderObject { VERTEX_SHADER, llgl::opengl::ShaderObject::Vertex },
            llgl::opengl::ShaderObject { FRAGMENT_SHADER, llgl::opengl::ShaderObject::Fragment }
        };
        program.construct(objects);
    }
    return *program.ptr();
}

class BlurShader : public llgl::opengl::Shader {
public:
    BlurShader()
        : llgl::opengl::Shader(shader())
    {
    }

    virtual llgl::opengl::AttributeMapping attribute_mapping() const override
    {
        return { .position = 1 };
    }

    void set_framebuffer_size(llgl::Vector2f size) { m_framebuffer_size = size; }

private:
    virtual void on_bind(llgl::opengl::ShaderScope& scope) const override
    {
        scope.set_uniform("accum", llgl::opengl::ShaderScope::CurrentTexture);
        scope.set_uniform("pass1", 1);
        scope.set_uniform("fbSize", m_framebuffer_size);
    }

    llgl::Vector2f m_framebuffer_size;
};

llgl::Vector2f next_oscilloscope_position()
{
    static float angle = 0;
    angle += 0.7;
    return llgl::Vector2f { std::sin(angle), std::cos(angle) } * 20
        + llgl::Vector2f { llgl::mouse_position() };
}

int main()
{
    llgl::Window window({ 512, 512 }, u8"Oscilloscope");
    llgl::opengl::enable_debug_output();

    auto& renderer = window.renderer();
    llgl::opengl::shaders::Basic330Core basic_shader;
    BlurShader blur_shader;

    llgl::RenderToTexture pass1 { window };
    pass1.set_label("pass1");
    llgl::RenderToTexture accum { window };
    accum.set_label("accum");

    llgl::opengl::VAO fullscreen_vao;
    fullscreen_vao.load_vertexes(
        { { llgl::Vertex { .position = { -1, -1, 0 }, .color = llgl::Colors::white, .tex_coord = { 0, 1 } },
            llgl::Vertex { .position = { 1, -1, 0 }, .color = llgl::Colors::white, .tex_coord = { 1, 1 } },
            llgl::Vertex { .position = { -1, 1, 0 }, .color = llgl::Colors::white, .tex_coord = { 0, 0 } },
            llgl::Vertex { .position = { 1, 1, 0 }, .color = llgl::Colors::white, .tex_coord = { 1, 0 } } } });

    auto old_oscilloscope_position = next_oscilloscope_position();

    for (;;) {
        llgl::Event event;
        while (window.poll_event(event)) {
            switch (event.type) {
                default:
                    break;
            }
        }

        renderer.clear(llgl::Colors::black);

        llgl::View view;
        view.set_viewport(window.rect());
        blur_shader.set_framebuffer_size(llgl::Vector2f { window.size() });

        {
            // Draw the first (non-blurred) pass
            pass1.clear(llgl::Color { 0, 0, 0 });

            view.set_ortho({ { 0, 0, static_cast<double>(window.size().x), static_cast<double>(window.size().y) } });
            pass1.apply_view(view);

            auto oscilloscope_position = next_oscilloscope_position();

            constexpr float PointSize = 2;
            auto diff = oscilloscope_position - old_oscilloscope_position;
            if (llgl::vector::length_squared(diff) < PointSize * PointSize)
                diff = { PointSize, 0 };
            auto diff_norm = llgl::vector::normalize(diff);
            auto main = diff_norm * PointSize;
            auto cross = llgl::vector::perpendicular(diff_norm) * PointSize;

            llgl::opengl::VAO input_vao {
                { { llgl::Vertex { .position = llgl::Vector3f { old_oscilloscope_position - cross }, .color = llgl::Colors::green },
                    llgl::Vertex { .position = llgl::Vector3f { old_oscilloscope_position + cross }, .color = llgl::Colors::green },
                    llgl::Vertex { .position = llgl::Vector3f { old_oscilloscope_position + diff - cross }, .color = llgl::Colors::green },
                    llgl::Vertex { .position = llgl::Vector3f { old_oscilloscope_position + diff + cross }, .color = llgl::Colors::green } } }
            };

            pass1.draw_vao(input_vao, llgl::opengl::PrimitiveType::TriangleStrip, { .shader = &basic_shader });

            old_oscilloscope_position = oscilloscope_position;
        }

        {
            // Blur the pass1 and blend with accum
            // Do not clear because we want previous frames

            // TODO: Implement this in LLGL
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pass1.texture().id());
            glActiveTexture(GL_TEXTURE0);

            accum.draw_vao(fullscreen_vao, llgl::opengl::PrimitiveType::TriangleStrip, { .shader = &blur_shader, .texture = &accum.texture() });
        }

        // Draw the result to backbuffer
        llgl::View no_transform_view;
        no_transform_view.set_viewport(window.rect());
        no_transform_view.set_ortho({ { -1, -1, 2, 2 } });
        renderer.apply_view(no_transform_view);
        renderer.draw_vao(fullscreen_vao, llgl::opengl::PrimitiveType::TriangleStrip, { .shader = &basic_shader, .texture = &accum.texture() });
        window.display();
    }
    return 0;
}

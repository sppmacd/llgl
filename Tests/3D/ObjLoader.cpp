#include "LLGL/Core/Angle.hpp"
#include "LLGL/OpenGL/Shader.hpp"
#include <LLGL/3D/ObjLoader.hpp>
#include <LLGL/3D/Object3D.hpp>
#include <LLGL/OpenGL/Utils.hpp>
#include <LLGL/Renderer/BatchRenderer.hpp>
#include <LLGL/Renderer/Renderer.hpp>
#include <LLGL/Window/Window.hpp>
#include <iostream>

int main()
{
    llgl::Window window { { 500, 500 }, u8"OBJ loader", { 3, 2 } };

    llgl::opengl::enable(llgl::opengl::Feature::DepthTest);
    llgl::opengl::set_clear_color(llgl::Color { 255, 128, 128 });

    auto object = llgl::ObjLoader::load_object_from_file("../car.obj");
    if (!object.has_value())
    {
        std::cerr << "FAILED TO READ :((" << std::endl;
        return 1;
    }
    object.value().transform().translate({ -1.5, -1.5, -5 });
    object.value().transform().rotate_y(llgl::deg_to_rad(45));
    auto& shader = llgl::opengl::shaders::shade_flat();
    object.value().set_shader(shader);

    for (;;)
    {
        llgl::Event event;
        while (window.poll_event(event))
        {
        }
        llgl::opengl::clear(llgl::opengl::ClearMask::Color | llgl::opengl::ClearMask::Depth);

        llgl::View view;
        view.set_viewport(llgl::Recti { 0, 0, window.size().x, window.size().y });
        view.set_perspective({ 1.22, window.aspect(), 0.1, 20 });
        window.renderer().apply_view(view);
        object.value().transform().rotate_x(0.05);

        {
            llgl::opengl::ShaderScope scope(shader);
            scope.set_uniform("lightPos", llgl::Vector3f { 2, 4, 3 });
            scope.set_uniform("lightColor", llgl::Colors::red);
            window.renderer().render_object(object.value());
        }

        {
            object->transform().translate({ 3, 0, 0 });
            llgl::opengl::ShaderScope scope(shader);
            scope.set_uniform("lightPos", llgl::Vector3f { 2, 4, 3 });
            scope.set_uniform("lightColor", llgl::Colors::green);
            window.renderer().render_object(object.value());
            object->transform().translate({ -3, 0, 0 });
        }

        window.display();
    }
    return 0;
}

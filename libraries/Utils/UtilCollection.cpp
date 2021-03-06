#include "UtilCollection.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <ctime>
#include <iostream>
#include <sstream>
#include <future>
#include <glbinding/Binding.h>
#include <glbinding-aux/Meta.h>

namespace util
{
    std::string convertGLubyteToString(const GLubyte* content)
    {
        return std::string(reinterpret_cast<const char*>(content));
    }

    void printOpenGLInfo()
    {
        std::cout << "Renderer: " << convertGLubyteToString(glGetString(GL_RENDERER)) << std::endl;
        std::cout << "Vendor: " << convertGLubyteToString(glGetString(GL_VENDOR)) << std::endl;
        std::cout << "Version: " << convertGLubyteToString(glGetString(GL_VERSION)) << std::endl;
        std::cout << "Shading Language Version: " << convertGLubyteToString(glGetString(GL_SHADING_LANGUAGE_VERSION)) << std::endl;
    }

    GLFWwindow* setupGLFWwindow(unsigned int width, unsigned int height, std::string name)
    {
        glfwInit();

        if constexpr (debugmode)
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
        else
        {
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 0);
            glfwWindowHint(GLFW_CONTEXT_NO_ERROR, 1);
        }            

        glfwWindowHint(GLFW_SAMPLES, 1);

        GLFWwindow* window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(window);
        return window;
    }

    void initGL()
    {
        // init glbinding
        glbinding::Binding::initialize(glfwGetProcAddress);
    }

    std::vector<std::string> getGLExtenstions()
    {
        GLint nExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
        std::vector<std::string> extenstions;
        extenstions.reserve(nExtensions);
        for (int i = 0; i < nExtensions; i++)
        {
            extenstions.push_back(std::string(convertGLubyteToString(glGetStringi(GL_EXTENSIONS, i))));
        }
        return extenstions;
    }

    void getGLerror(int line, std::string function)
    {
        if constexpr(debugmode)
        {
            if (glfwGetCurrentContext() != nullptr)
            {
                GLenum err;
                while ((err = glGetError()) != GL_NO_ERROR)
                {
                    std::cout << "OpenGL Error: " << glbinding::aux::Meta::getString(err) << std::endl;
                    std::cout << "Last error check in function " << function << " at line " << line << std::endl;
                }
            }
        }
    }

    void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam)
    {
        // ignore unbound texture warning
        if (id == 131204)
            return;

        std::cout << "OpenGL debug callback:\n";
        std::cout << "Source:   " << glbinding::aux::Meta::getString(source)   << '\n';
        std::cout << "Message:  " << message                                         << '\n';
        std::cout << "Type:     " << glbinding::aux::Meta::getString(type)     << '\n';
        std::cout << "ID:       " << glbinding::aux::Meta::getString(id)      << '\n';
        std::cout << "severity: " << glbinding::aux::Meta::getString(severity) << '\n';

        std::cout << std::endl;
    }

    void enableDebugCallback()
    {
        if constexpr(debugmode)
        {
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(debugCallback, nullptr);

            // disable notifications and memory info
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);

            // enable more severe errors
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
        }
    }

    void savePNG(std::string name, std::vector<unsigned char>& image, int width, int height)
    {
        std::stringstream path;
        path << (util::gs_resourcesPath) << "../../" << name << "_" << time(nullptr) << ".png";

        stbi_flip_vertically_on_write(true);
        const auto err = stbi_write_png(path.str().c_str(), width, height, 4, image.data(), 4 * width);
        if (err == 0)
            throw std::runtime_error("error writing image");
    }

    void saveFBOtoFile(std::string name, GLFWwindow* window)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);
        std::vector<unsigned char> image;
        image.resize(width * height * 4);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

        //Encode the image
        try
        {
            auto future = std::async(std::launch::async, [&]() { savePNG(name, image, width, height); });
        }
        catch (std::runtime_error& ex)
        {
            std::cout << ex.what() << std::endl;
        }
    }
}

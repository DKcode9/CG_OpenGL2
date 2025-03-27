#include <Windows.h>
#include <iostream>
#include <limits>//library for infinity
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 512;
int Height = 512;
std::vector<float> OutputImage;
// -------------------------------------------------

// -------------------------------------------------
// Classes
// -------------------------------------------------
class Ray {
public:
    vec3 origin, direction;
    Ray(const vec3& o, const vec3& d) : origin(o), direction(normalize(d)) {}
};

class Surface {
public:
    vec3 color;
    Surface(const vec3& c) : color(c) {}
    virtual bool intersect(const Ray& ray, float& t, float tMin, float tMax) const = 0;
};

class Sphere : public Surface {
public:
    vec3 center;
    float radius;
    Sphere(const vec3& c, float r, const vec3& col)
        : Surface(col), center(c), radius(r) {
    }

    bool intersect(const Ray& ray, float& t, float tMin, float tMax) const override {
        vec3 oc = ray.origin - center;
        float a = dot(ray.direction, ray.direction);
        float b = dot(oc, ray.direction);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - c;//a is 1

        if (discriminant > 0) {
            float temp = (-b - std::sqrt(discriminant)) / a;
            if (temp < tMax && temp > tMin) {
                t = temp;
                return true;
            }
        }
        return false;
    }
};

class Plane : public Surface {
public:
    vec3 normal;
    float d;

    Plane(const vec3& n, float d, const vec3& col)
        : Surface(col), normal(normalize(n)), d(d) {
    }

    bool intersect(const Ray& ray, float& t, float tMin, float tMax) const override {
        float denom = dot(normal, ray.direction);
        if (abs(denom) > 1e-6) { // Ensure the ray is not parallel to the plane
            t = (d - dot(normal, ray.origin)) / denom;
            return (t >= tMin && t <= tMax);
        }
        return false;
    }
};


class Camera {
public:
    vec3 position, direction;
	float focalLength;
    Camera(const vec3& pos, const vec3& dir,const float& focal)
        : position(pos), direction(normalize(dir)),focalLength(focal) {
    }

    Ray getRay(float x, float y) {
        return Ray(position, normalize(vec3(x, y, 0.0f) + vec3(0, 0, -0.1f))); // image plane is at -0.1
    }
};

class Scene {
public:
    std::vector<Surface*> surfaces;
    Camera camera;

    Scene(const std::vector<Surface*>& sur, const Camera& c)
        : surfaces(sur), camera(c) {
    }

    vec3 trace(const Ray& ray, float tMin, float tMax) {
        float t = tMax;
        const Surface* hitSurface = nullptr;

        for (const auto& surface : surfaces) {
            float tempT;
            if (surface->intersect(ray, tempT, tMin, tMax) && tempT < t) {
                t = tempT;
                hitSurface = surface;
            }
        }
        return (hitSurface != nullptr) ? hitSurface->color : vec3(0.0f, 0.0f, 0.0f); //if null -> black color
    }
};

class ImagePlane {
public:
    void set(int x, int y, vec3 color) {
        OutputImage[(y * Width + x) * 3] = color.x;
        OutputImage[(y * Width + x) * 3 + 1] = color.y;
        OutputImage[(y * Width + x) * 3 + 2] = color.z;
    }
};

void render() {
    OutputImage.resize(Width * Height * 3, 1.0f);
    vec3 wcolor = vec3(1.0f, 1.0f, 1.0f); // sphere color

    Sphere sphere1(vec3(-4.0f, 0.0f, -7.0f), 1.0f, wcolor); // sphere1 define
    Sphere sphere2(vec3(0.0f, 0.0f, -7.0f), 2.0f, wcolor); // sphere2 define
    Sphere sphere3(vec3(4.0f, 0.0f, -7.0f), 1.0f, wcolor); // sphere3 define
    Plane plane(vec3(0.0f, 1.0f, 0.0f), -2.0f, wcolor); // plane define

    Camera camera(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),0.1f);
    std::vector<Surface*> surfaces = { &sphere1, &sphere2, &sphere3, &plane };
    Scene scene(surfaces, camera);
    ImagePlane image;

    for (int iy = 0; iy < Height; ++iy) {
        for (int ix = 0; ix < Width; ++ix) {
            float x = (0.2f * (ix + 0.5f)) / Width - 0.1f;
            float y = (0.2f * (iy + 0.5f)) / Height - 0.1f;
            Ray ray = scene.camera.getRay(x, y);
            vec3 color = scene.trace(ray, 0.0f, FLT_MAX);
            image.set(ix, iy, color);
        }
    }
}

void resize_callback(GLFWwindow*, int nw, int nh)
{
    //This is called in response to the window resizing.
    //The new width and height are passed in so we make 
    //any necessary changes:
    Width = nw;
    Height = nh;
    //Tell the viewport to use all of our screen estate
    glViewport(0, 0, nw, nh);

    //This is not necessary, we're just working in 2d so
    //why not let our spaces reflect it?
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0, static_cast<double>(Width)
        , 0.0, static_cast<double>(Height)
        , 1.0, -1.0);

    //Reserve memory for our render so that we don't do 
    //excessive allocations and render the image
    OutputImage.reserve(Width * Height * 3);
    render();
}

int main(int argc, char* argv[])
{
    // -------------------------------------------------
    // Initialize Window
    // -------------------------------------------------

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    //We have an opengl context now. Everything from here on out 
    //is just managing our window or opengl directly.

    //Tell the opengl state machine we don't want it to make 
    //any assumptions about how pixels are aligned in memory 
    //during transfers between host and device (like glDrawPixels(...) )
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    //We call our resize function once to set everything up initially
    //after registering it as a callback with glfw
    glfwSetFramebufferSizeCallback(window, resize_callback);
    resize_callback(NULL, Width, Height);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        //Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // -------------------------------------------------------------
        //Rendering begins!
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
        //and ends.
        // -------------------------------------------------------------

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        //Close when the user hits 'q' or escape
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
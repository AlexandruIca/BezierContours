#include <SDL.h>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <array>
#include <chrono>

#define INFO(...) spdlog::info(__VA_ARGS__)
#define FATAL(...) spdlog::error(__VA_ARGS__)

struct program_description
{
    std::string vertex_shader_source;
    std::string fragment_shader_source;
};

[[nodiscard]] auto create_shader(std::string const& source, unsigned int const type) -> unsigned int
{
    unsigned int shader = glCreateShader(type);

    char const* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int succeded = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succeded);

    if(succeded != GL_TRUE) {
        int len = 0;
        std::string msg;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        msg.resize(static_cast<std::size_t>(len));
        glGetShaderInfoLog(shader, static_cast<int>(msg.size()), nullptr, msg.data());

        std::string const shader_type = (type == GL_VERTEX_SHADER ? "vertex" : "fragment");
        FATAL("Compilation of {} shader failed: {}", shader_type, msg);
    }

    return shader;
}

[[nodiscard]] auto create_program(program_description const& desc) -> unsigned int
{
    unsigned int program = glCreateProgram();

    auto const vs = create_shader(desc.vertex_shader_source, GL_VERTEX_SHADER);
    auto const fs = create_shader(desc.fragment_shader_source, GL_FRAGMENT_SHADER);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int succeded = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &succeded);

    if(succeded != GL_TRUE) {
        int len = 0;
        std::string msg;

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        msg.resize(static_cast<std::size_t>(len));
        glGetProgramInfoLog(program, static_cast<int>(msg.size()), nullptr, msg.data());

        FATAL("Program could not be linked: {}", msg);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

auto set_mat4(unsigned int program, std::string const& var, glm::mat4 const& mat) noexcept -> void
{
    glUniformMatrix4fv(glGetUniformLocation(program, var.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

int width = 800;
int height = 800;

glm::mat4 rotation{ 1.0F };
glm::mat4 projection{ 1.0F };
float fov = 45.0F;
glm::vec3 translation{ 0.0F, 0.0F, -2.0F };
glm::vec3 scale{ 1.0F, 1.0F, 1.0F };

auto rotate(float const angle, glm::vec3 const& axis) noexcept -> void
{
    rotation *= glm::toMat4(glm::angleAxis(angle, axis));
}

auto handle_events(SDL_Window* window, bool& running, unsigned int const program, float const duration) -> void
{
    static float translate_offset = 1.5F;
    constexpr float scale_offset = 2.0F;
    constexpr float rotate_offset = 2.0F;

    SDL_Event ev;
    while(SDL_PollEvent(&ev)) {
        if(ev.type == SDL_QUIT) {
            running = false;
            break;
        }
        if(ev.window.windowID != SDL_GetWindowID(window)) {
            continue;
        }

        switch(ev.type) {
        case SDL_WINDOWEVENT: {
            if(ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                width = ev.window.data1;
                height = ev.window.data2;
                glViewport(0, 0, width, height);
                projection = glm::perspective(fov, (width * 1.0F) / (height * 1.0F), 0.1F, 100.0F);
                set_mat4(program, "u_projection", projection);
                INFO("Window resize: w={}, h={}", width, height);
            }
            break;
        }
        case SDL_KEYDOWN: {
            switch(ev.key.keysym.sym) {
            case SDLK_ESCAPE: {
                running = false;
                break;
            }
            case SDLK_UP: {
                translation.y -= translate_offset * duration;
                break;
            }
            case SDLK_DOWN: {
                translation.y += translate_offset * duration;
                break;
            }
            case SDLK_LEFT: {
                translation.x += translate_offset * duration;
                break;
            }
            case SDLK_RIGHT: {
                translation.x -= translate_offset * duration;
                break;
            }
            case SDLK_j: {
                scale.x += scale_offset * duration;
                break;
            }
            case SDLK_k: {
                scale.x -= scale_offset * duration;
                break;
            }
            case SDLK_l: {
                scale.y -= scale_offset * duration;
                break;
            }
            case SDLK_SEMICOLON: {
                scale.y += scale_offset * duration;
                break;
            }
            case SDLK_q: {
                scale.x -= scale_offset * duration;
                scale.y -= scale_offset * duration;
                break;
            }
            case SDLK_e: {
                scale.x += scale_offset * duration;
                scale.y += scale_offset * duration;
                break;
            }
            case SDLK_a: {
                rotate(rotate_offset * duration, glm::vec3{ 0.0F, 1.0F, 0.0F });
                break;
            }
            case SDLK_d: {
                rotate(-rotate_offset * duration, glm::vec3{ 0.0F, 1.0F, 0.0F });
                break;
            }
            default: {
                break;
            }
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            fov -= ev.wheel.y * duration;
            translate_offset -= ev.wheel.y * duration * 1.5F;
            fov = std::clamp(fov, 44.0F, 46.7F);
            projection = glm::perspective(fov, (width * 1.0F) / (height * 1.0F), 0.1F, 100.0F);
            set_mat4(program, "u_projection", projection);
            break;
        }
        default: {
            break;
        }
        }
    }
}

auto main() noexcept -> int
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        FATAL("Couldn't initialize SDL!");
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("Bezier",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          width,
                                          height,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_GLContext ctx = SDL_GL_CreateContext(window);

    SDL_GL_SetSwapInterval(1);

    if(!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        FATAL("Couldn't create OpenGL context!");
    }

    INFO("OpenGL context created! Version {}.{}!", GLVersion.major, GLVersion.minor);

    program_description desc;

    desc.vertex_shader_source = R"(
    #version 330 core

    layout(location = 0) in vec2 pos;
    layout(location = 1) in vec2 coord;
    layout(location = 2) in vec4 color;

    uniform mat4 u_model;
    uniform mat4 u_projection;

    out vec2 o_coord;
    out vec4 o_color;

    void main() {
        gl_Position = u_projection * u_model * vec4(pos.xy, 0.0, 1.0);
        o_coord = coord;
        o_color = color;
    }
    )";

    desc.fragment_shader_source = R"(
    #version 330 core

    in vec2 o_coord;
    in vec4 o_color;
    out vec4 frag_color;

    #define NUM_BEZIER_CURVES 6
    #define NUM_CONTROL_POINTS 3

    uniform vec2 u_curves[NUM_BEZIER_CURVES * NUM_CONTROL_POINTS];

    float eval_curve(float y1, float y2, float y3, float t) {
        float mt = 1.0 - t;
        return mt * mt * y1 + 2.0 * t * mt * y2 + t * t * y3;
    }

    void main() {
        float coverage = 0.0;
        vec2 ppem = vec2(1.0 / fwidth(o_coord.x), 1.0 / fwidth(o_coord.y));

        for(int i = 0; i < NUM_BEZIER_CURVES * NUM_CONTROL_POINTS; i += NUM_CONTROL_POINTS) {
            vec2 p1 = u_curves[i] - o_coord;
            vec2 p2 = u_curves[i + 1] - o_coord;
            vec2 p3 = u_curves[i + 2] - o_coord;

            float a = p1.y - 2 * p2.y + p3.y;
            float b = p1.y - p2.y;
            float c = p1.y;

            float t1 = 0.0;
            float t2 = 0.0;

            if(abs(a) < 0.0001) {
                t1 = c / (2.0 * b);
                t2 = c / (2.0 * b);
            }
            else {
                float root = sqrt(max(b * b - a * c, 0.0));
                t1 = (b - root) / a;
                t2 = (b + root) / a;
            }

            int num = ((p1.y > 0.0) ? 2 : 0) + ((p2.y > 0.0) ? 4 : 0) + ((p3.y > 0.0) ? 8 : 0);
            int sh = 0x2E74 >> num;

            if((sh & 1) != 0) {
                float r1 = eval_curve(p1.x, p2.x, p3.x, t1);
                coverage += clamp(r1 * ppem.x + 0.5, 0.0, 1.0);
            }
            if((sh & 2) != 0) {
                float r2 = eval_curve(p1.x, p2.x, p3.x, t2);
                coverage -= clamp(r2 * ppem.x + 0.5, 0.0, 1.0);
            }
        }

        float coverage2 = 0.0;

        for(int i = 0; i < NUM_BEZIER_CURVES * NUM_CONTROL_POINTS; i += NUM_CONTROL_POINTS) {
            vec2 p1 = u_curves[i].yx - o_coord.yx;
            vec2 p2 = u_curves[i + 1].yx - o_coord.yx;
            vec2 p3 = u_curves[i + 2].yx - o_coord.yx;

            float a = p1.y - 2 * p2.y + p3.y;
            float b = p1.y - p2.y;
            float c = p1.y;

            float t1 = 0.0;
            float t2 = 0.0;

            if(abs(a) < 0.0001) {
                t1 = c / (2.0 * b);
                t2 = c / (2.0 * b);
            }
            else {
                float root = sqrt(max(b * b - a * c, 0.0));
                t1 = (b - root) / a;
                t2 = (b + root) / a;
            }

            int num = ((p1.y > 0.0) ? 2 : 0) + ((p2.y > 0.0) ? 4 : 0) + ((p3.y > 0.0) ? 8 : 0);
            int sh = 0x2E74 >> num;

            if((sh & 1) != 0) {
                float r1 = eval_curve(p1.x, p2.x, p3.x, t1);
                coverage2 += clamp(r1 * ppem.y + 0.5, 0.0, 1.0);
            }
            if((sh & 2) != 0) {
                float r2 = eval_curve(p1.x, p2.x, p3.x, t2);
                coverage2 -= clamp(r2 * ppem.y + 0.5, 0.0, 1.0);
            }
        }

        float coverage_h = min(abs(coverage), 1.0);
        float coverage_v = min(abs(coverage2), 1.0);
        float avg_coverage = (coverage_h + coverage_v) / 2.0;
        frag_color = vec4(o_color * avg_coverage);
    }
    )";

    std::vector<float> const curves = {
        0.3F,  0.3F, 0.5F,  0.5F, 0.3F,  0.7F, // first curve
        0.3F,  0.7F, 1.0F,  0.5F, 0.3F,  0.3F, // second curve
        0.9F,  0.3F, 0.9F,  0.5F, 0.9F,  0.7F, // third curve
        0.9F,  0.7F, 0.93F, 0.7F, 0.95F, 0.7F, // fourth curve
        0.95F, 0.7F, 0.95F, 0.5F, 0.95F, 0.3F, // fifth curve
        0.95F, 0.3F, 0.93F, 0.3F, 0.9F,  0.3F  // sixth curve
    };

    auto const program = create_program(desc);
    glUseProgram(program);

    projection = glm::perspective(fov, (width * 1.0F) / (height * 1.0F), 0.1F, 100.0F);
    set_mat4(program, "u_projection", projection);

    glUniform2fv(glGetUniformLocation(program, "u_curves"), curves.size() / 2, curves.data());

    unsigned int vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glm::vec4 const color{ 1.0F, 128.0F / 255.0F, 64.0F / 255.0F, 1.0F };

    std::array<float, 32> const vertices = {
        -1.0F, 1.0F,  0.0F, 0.0F, color.r, color.g, color.b, color.a, // TOP LEFT
        1.0F,  1.0F,  1.0F, 0.0F, color.r, color.g, color.b, color.a, // TOP RIGHT
        1.0F,  -1.0F, 1.0F, 1.0F, color.r, color.g, color.b, color.a, // BOTTOM RIGHT
        -1.0F, -1.0F, 0.0F, 1.0F, color.r, color.g, color.b, color.a  // BOTTOM LEFT
    };

    std::array<unsigned int, 6> const indices = { 0, 1, 2, 2, 3, 0 };

    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int ibo = 0;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnable(GL_MULTISAMPLE);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    bool running = true;

    using namespace std::chrono;
    auto start = steady_clock::now();

    while(running) {
        auto end = steady_clock::now();
        auto const duration = duration_cast<milliseconds>(end - start).count() / 1000.0F;
        start = end;
        glm::mat4 model{ 1.0F };
        model = glm::translate(model, translation);
        model *= rotation;
        model = glm::scale(model, scale);
        set_mat4(program, "u_model", model);

        handle_events(window, running, program, duration);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

        SDL_GL_SwapWindow(window);
    }

    glDeleteBuffers(1, &ibo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

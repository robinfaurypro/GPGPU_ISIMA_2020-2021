#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <random>
#include <string>

const char *kVertexSource = R"()";

const char *kFragmentSource = R"()";

struct Content {
  GLuint kernel_draw_fractal_;
};

void Initialization(Content& content) {
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &kVertexSource, nullptr);
  glCompileShader(vertex_shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint length;
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
      std::vector<GLchar> log(length+1, 0);
      glGetShaderInfoLog(vertex_shader, length, nullptr, log.data());
      throw std::runtime_error(
        std::string("[ERROR] Vertex shader compilation")+
        std::string(log.data()));
    }
  }

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &kFragmentSource, nullptr);
  glCompileShader(fragment_shader);
  ok = GL_FALSE;
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint length;
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
      std::vector<GLchar> log(length+1, 0);
      glGetShaderInfoLog(fragment_shader, length, nullptr, log.data());
      throw std::runtime_error(
        std::string("[ERROR] Fragment shader compilation")+
        std::string(log.data()));
    }
  }

  content.kernel_draw_fractal_ = glCreateProgram();
  glAttachShader(content.kernel_draw_fractal_, vertex_shader);
  glAttachShader(content.kernel_draw_fractal_, fragment_shader);
  glLinkProgram(content.kernel_draw_fractal_);
  ok = GL_FALSE;
  glGetProgramiv(content.kernel_draw_fractal_, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint length;
    glGetProgramiv(
      content.kernel_draw_fractal_, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
      std::vector<GLchar> log(length+1, 0);
      glGetProgramInfoLog(
        content.kernel_draw_fractal_, length, nullptr, log.data());
      throw std::runtime_error(
        std::string("[ERROR] Program draw link fail")+
        std::string(log.data()));
    }
  }
  glDetachShader(content.kernel_draw_fractal_, vertex_shader);
  glDetachShader(content.kernel_draw_fractal_, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void ComputeFractal(Content& content) {
  
}

void ComputeFrame(Content& content) {
  ComputeFractal(content);

  glClearColor(0.16, 0.16, 0.16, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  /*const uint64_t stride = sizeof(Particle);
  glUseProgram(content.kernel_draw_particles_);
  glBindBuffer(GL_ARRAY_BUFFER, content.buffer_);
  glVertexAttribPointer(
    0, 2, GL_FLOAT, GL_FALSE, stride, (void*)(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
    1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*(2)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*(2+2)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
    3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*(2+2+3)));
  glEnableVertexAttribArray(3);
  glDrawArrays(GL_POINTS, 0, content.particles_.size());
  glDisableVertexAttribArray(3);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);*/
}

void Destroy(Content& content) {
  glDeleteProgram(content.kernel_draw_fractal_);
}

void main() {
  if (!glfwInit()) {
    throw std::runtime_error("[ERROR] init GLFW");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window =
    glfwCreateWindow(800, 800, "ISIMA_Practical_1", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("[ERROR] Window creation");
  }

  glfwMakeContextCurrent(window);

  glewExperimental = GL_TRUE;
  GLenum error = glewInit();
  if (error != GLEW_OK) {
    glfwTerminate();
    throw std::runtime_error(
      std::string("[ERROR] init GLEW")+(const char*)glewGetErrorString(error));
  }

  glfwSwapInterval(false);

  std::cout<<"OpenGL version "<<glGetString(GL_VERSION)<<std::endl;
  std::cout<<"Device: "<<glGetString(GL_RENDERER)<<std::endl;

  double last_time = glfwGetTime();
  int frames_cmp = 0;
  bool running = true;
  Content content;

  Initialization(content);

  GLuint VAO = 0;
  glGenVertexArrays(1, &VAO);

  while (running) {
    frames_cmp++;
    if (glfwGetTime()-last_time >= 1.0) {
      std::cout<<1000.0/double(frames_cmp)<<" ms/frame"<<std::endl;
      frames_cmp = 0;
      last_time += 1.0;
    }

    if (glfwWindowShouldClose(window)) {
      running = false;
    }

    glBindVertexArray(VAO);
    ComputeFrame(content);
    glBindVertexArray(0);

    GLuint OpenGL_error = glGetError();
    if (OpenGL_error) {
      throw std::runtime_error(
        std::string("[ERROR] init GLEW")+std::to_string(OpenGL_error));
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  Destroy(content);

  glfwTerminate();
}

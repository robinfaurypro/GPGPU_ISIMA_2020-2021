#include <algorithm>
#include <chrono>
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <random>
#include <stack>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const char *kVertexSource = R"(
#version 430 core

out vec2 uv;

void main() {
  uv = vec2(                                 // Generate a quad using two
    float(((uint(gl_VertexID)+2u)/3u)%2u),   // triangles to fill the space
    float(((uint(gl_VertexID)+1u)/3u)%2u));  // ([0,1],[0,1])
  gl_Position = vec4(uv*2.0-1.0, 0.0, 1.0);  // OpenGL space is ([-1,1],[-1,1])
  uv = vec2(uv.x, 1.0-uv.y);                 // OpenGL space is fliped in Y
})";

const char *kFragmentSource = R"(
#version 430 core

in vec2 uv;
uniform sampler2D texture_;
uniform int has_texture_ = 0;

out vec4 output_color;

void main() {
  vec3 color = vec3(uv, 0.0);
  if (has_texture_ == 1) {
    color = vec3(texture(texture_, uv).rgb);
  }
  output_color = vec4(color, 1.0);
})";

struct Content {
  int32_t width = 1024;
  int32_t height = 1024;
  std::vector<uint8_t> image_original_;
  std::vector<uint8_t> image_data_color_;
  std::vector<uint8_t> image_data_grayscale_;
  std::vector<std::pair<int32_t, int32_t>> seeds_;
  int32_t cell_count_ = 0;

  GLuint kernel_draw_image_ = 0;
  GLuint texture_ = 0;
};

void Initialization(Content& content) {
  int32_t planes;
  std::string image_path = std::string(RESOURCES_PATH)+"/input_data.png";
  uint8_t* image_data_raw =
    stbi_load(image_path.c_str(), &content.width, &content.width, &planes, 3);
  content.image_original_.resize(content.width*content.height*3);
  memcpy(
    content.image_original_.data(),
    image_data_raw,
    content.image_original_.size());
  delete image_data_raw;

  content.image_data_color_ = content.image_original_;

  glGenTextures(1, &content.texture_);
  glBindTexture(GL_TEXTURE_2D, content.texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGB8,
    content.width,
    content.height,
    0,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);

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

  content.kernel_draw_image_ = glCreateProgram();
  glAttachShader(content.kernel_draw_image_, vertex_shader);
  glAttachShader(content.kernel_draw_image_, fragment_shader);
  glLinkProgram(content.kernel_draw_image_);
  ok = GL_FALSE;
  glGetProgramiv(content.kernel_draw_image_, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint length;
    glGetProgramiv(
      content.kernel_draw_image_, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
      std::vector<GLchar> log(length+1, 0);
      glGetProgramInfoLog(
        content.kernel_draw_image_, length, nullptr, log.data());
      throw std::runtime_error(
        std::string("[ERROR] Program draw link fail")+
        std::string(log.data()));
    }
  }
  glDetachShader(content.kernel_draw_image_, vertex_shader);
  glDetachShader(content.kernel_draw_image_, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void GrayscaleConversion(Content& content) {
  content.image_data_grayscale_.resize(content.width*content.height);
  for (int32_t x=0; x<content.width; ++x) {
    for (int32_t y=0; y<content.height; ++y) {
      uint32_t sum = 0;
      sum += content.image_data_color_[y*content.width*3+x*3+0];
      sum += content.image_data_color_[y*content.width*3+x*3+1];
      sum += content.image_data_color_[y*content.width*3+x*3+2];
      content.image_data_grayscale_[y*content.width+x] = sum/3;
    }
  }
}

void BlurImage(Content& content) {
  std::vector<uint8_t> image_blurred(content.width*content.height, 0u);
  std::vector<uint8_t>& image_grayscale = content.image_data_grayscale_;
  for (int32_t x=1; x<content.width-1; ++x) {
    for (int32_t y=1; y<content.height-1; ++y) {
      int32_t sum = 0;
      sum += image_grayscale[(y-1)*content.width+(x-1)];
      sum += image_grayscale[(y-1)*content.width+(x+0)];
      sum += image_grayscale[(y-1)*content.width+(x+1)];
      sum += image_grayscale[(y+0)*content.width+(x-1)];
      sum += image_grayscale[(y+0)*content.width+(x+0)];
      sum += image_grayscale[(y+0)*content.width+(x+1)];
      sum += image_grayscale[(y+1)*content.width+(x-1)];
      sum += image_grayscale[(y+1)*content.width+(x+0)];
      sum += image_grayscale[(y+1)*content.width+(x+1)];
      image_blurred[y*content.width+x] = static_cast<uint8_t>(sum/9);
    }
  }
  content.image_data_grayscale_ = image_blurred;
}

void ContourDetection(Content& content) {
  const float Sobel_x[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
  };
  const float Sobel_y[3][3] = {
    {-1, -2, -1},
    {0, 0, 0},
    {1, 2, 1}
  };

  std::vector<uint8_t> contour(content.width*content.height, 0u);
  std::vector<uint8_t>& image_grayscale = content.image_data_grayscale_;
  int32_t max_value = -1000;
  int32_t min_value = 1000;
  for (int32_t x=1; x<content.width-1; ++x) {
    for (int32_t y=1; y<content.height-1; ++y) {
      int32_t Gx =
        image_grayscale[(y-1)*content.width+(x-1)]+
        2*image_grayscale[(y)*content.width+(x-1)]+
        image_grayscale[(y+1)*content.width+(x-1)]-
        image_grayscale[(y-1)*content.width+(x+1)]-
        2*image_grayscale[(y)*content.width+(x+1)]-
        image_grayscale[(y+1)*content.width+(x+1)];

      int32_t Gy =
        image_grayscale[(y-1)*content.width+(x-1)]+
        2*image_grayscale[(y-1)*content.width+(x)]+
        image_grayscale[(y-1)*content.width+(x+1)]-
        image_grayscale[(y+1)*content.width+(x-1)]-
        2*image_grayscale[(y+1)*content.width+(x)]-
        image_grayscale[(y+1)*content.width+(x+1)];
      
      int32_t value = static_cast<uint8_t>(std::sqrt(Gx*Gx+Gy*Gy));

      contour[y*content.width+x] = value;
    }
  }

  content.image_data_grayscale_ = contour;
  for (int32_t x=0; x<content.width; ++x) {
    content.image_data_grayscale_[x] =
      content.image_data_grayscale_[content.width+x];
    content.image_data_grayscale_[(content.height-1)*content.width+x] =
      content.image_data_grayscale_[(content.height-2)*content.width+x];
  }
  for (int32_t y=0; y<content.height; ++y) {
    content.image_data_grayscale_[y*content.width] =
      content.image_data_grayscale_[y*content.width+1];
    content.image_data_grayscale_[y*content.width+content.width-1] =
      content.image_data_grayscale_[y*content.width+content.width-2];
  }
}

void ApplyLevel(Content& content) {
  std::vector<uint8_t>& image_grayscale = content.image_data_grayscale_;
  for (int32_t x=0; x<content.width; ++x) {
    for (int32_t y=0; y<content.height; ++y) {
      uint8_t& pixel_color = image_grayscale[(y)*content.width+(x)];
      if (pixel_color < 32) {
        pixel_color = 0u;
      } else {
        pixel_color = 255u;
      }
    }
  }
}

void ClearImage(Content& content) {
  std::fill(
    content.image_data_color_.begin(), content.image_data_color_.end(), 0);
}

void AddSeeds(Content& content) {
  content.seeds_.clear();
  content.seeds_.resize(1024);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> d(0, 1);
  for (int32_t i = 0; i<content.seeds_.size(); ++i) {
    int32_t x = d(gen)*content.width;
    int32_t y = d(gen)*content.height;
    if (content.image_data_grayscale_[(y)*content.width+(x)] == 0) {
      content.seeds_[i] = std::make_pair(x, y);
      content.image_data_color_[y*content.width*3+x*3+0] = (d(gen)*0.9f+0.1f)*255;
      content.image_data_color_[y*content.width*3+x*3+1] = (d(gen)*0.9f+0.1f)*255;
      content.image_data_color_[y*content.width*3+x*3+2] = (d(gen)*0.9f+0.1f)*255;
    }
  }
}

void FloodFill(Content& content) {
  for (std::pair<int32_t, int32_t>& seed : content.seeds_) {
    std::stack<std::pair<int32_t, int32_t>> neighborhood;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    {
      const int32_t x = seed.first;
      const int32_t y = seed.second;
      r = content.image_data_color_[y*content.width*3+x*3+0];
      g = content.image_data_color_[y*content.width*3+x*3+1];
      b = content.image_data_color_[y*content.width*3+x*3+2];
      neighborhood.push(std::make_pair(x-1, y));
      neighborhood.push(std::make_pair(x+1, y));
      neighborhood.push(std::make_pair(x, y-1));
      neighborhood.push(std::make_pair(x, y+1));
    }

    while (!neighborhood.empty()) {
      const int32_t x = neighborhood.top().first;
      const int32_t y = neighborhood.top().second;
      neighborhood.pop();
      if (x < 0 || x >= content.width || y < 0 || y >= content.height) {
        continue;
      }
      if (content.image_data_grayscale_[(y)*content.width+(x)] > 0) {
        continue;
      }
      uint8_t image_color[3] = {
        content.image_data_color_[y*content.width*3+x*3+0],
        content.image_data_color_[y*content.width*3+x*3+1],
        content.image_data_color_[y*content.width*3+x*3+2]
      };
      if (image_color[0] == r && image_color[1] == g && image_color[2] == b) {
        continue;
      }
      if ((image_color[0] != 0 || image_color[1] != 0 || image_color[2] != 0)&&
          !(image_color[0] == r && image_color[1] == g && image_color[2] == b)) {
        content.seeds_.erase(
          std::remove(
            content.seeds_.begin(), content.seeds_.end(), std::make_pair(x, y)),
          content.seeds_.end());
      }
      content.image_data_color_[y*content.width*3+x*3+0] = r;
      content.image_data_color_[y*content.width*3+x*3+1] = g;
      content.image_data_color_[y*content.width*3+x*3+2] = b;
      neighborhood.push(std::make_pair(x-1, y));
      neighborhood.push(std::make_pair(x+1, y));
      neighborhood.push(std::make_pair(x, y-1));
      neighborhood.push(std::make_pair(x, y+1));
    }
  }
}

void ComputeHistogram(Content& content) {
  std::vector<uint8_t> red(255, 0);
  std::vector<uint8_t> green(255, 0);
  std::vector<uint8_t> blue(255, 0);
  for (int32_t x=0; x<content.width; ++x) {
    for (int32_t y=0; y<content.height; ++y) {
      ++red[content.image_data_color_[y*content.width*3+x*3+0]];
      ++green[content.image_data_color_[y*content.width*3+x*3+1]];
      ++blue[content.image_data_color_[y*content.width*3+x*3+2]];
    }
  }

  content.cell_count_ = 0;
  for (uint8_t r : red) {
    if (r > 0) {
      ++content.cell_count_;
    }
  }
}

void SendTextureToGPU(Content& content) {
  glBindTexture(GL_TEXTURE_2D, content.texture_);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGB8,
    content.width,
    content.height,
    0,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    content.image_data_color_.data());
  glBindTexture(GL_TEXTURE_2D, 0);
}

void ComputeFrame(Content& content) {
  content.image_data_color_ = content.image_original_;
  GrayscaleConversion(content);
  ClearImage(content);
  BlurImage(content);
  ContourDetection(content);
  ApplyLevel(content);
  AddSeeds(content);
  FloodFill(content);
  ComputeHistogram(content);
  SendTextureToGPU(content);

  glClearColor(0.16, 0.16, 0.16, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(content.kernel_draw_image_);
  if (content.texture_ != 0) {
    glBindTexture(GL_TEXTURE_2D, content.texture_);
    glUniform1i(
      glGetUniformLocation(
        content.kernel_draw_image_, "has_texture_"), 1);
    glUniform1i(
      glGetUniformLocation(content.kernel_draw_image_, "texture_"), 0);
  }
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

void Destroy(Content& content) {
  glDeleteProgram(content.kernel_draw_image_);
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
    glfwCreateWindow(800, 800, "ISIMA_Practical_Marked", nullptr, nullptr);
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

  bool running = true;
  Content content;

  Initialization(content);

  GLuint VAO = 0;
  glGenVertexArrays(1, &VAO);

  while (running) {
    if (glfwWindowShouldClose(window)) {
      running = false;
    }

    auto start = std::chrono::steady_clock::now(); // From https://en.cppreference.com/w/cpp/chrono
    glBindVertexArray(VAO);
    ComputeFrame(content);
    glBindVertexArray(0);
    std::cout << "Cell count: " << content.cell_count_ << std::endl;
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s" << std::endl << std::endl;

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

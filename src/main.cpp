#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <boost/program_options.hpp>
#include <boost/scope/scope_exit.hpp>
#include <GLFW/glfw3.h>
#include "platform/framebuffer_presenter.h"
#include "render/framebuffer.h"
#include "resources/assets.h"
#include "resources/cockpit_mask.h"

namespace {

int startup_failure(std::string const &message) {
  /// Report actionable startup errors while leaving unexpected engine failures uncaught
  std::cerr << "ERROR: " << message << std::endl;
  return EXIT_FAILURE;
}

} // anonymous namespace

auto main(int const argc, char const *const argv[])->int {
  /// Bring up the native display and first embedded resource before reconstructing flight
  boost::program_options::options_description options{"Corncob 3D: reconstruction in progress"};
  options.add_options()
    ("help,h", "show command-line options")
    ("version", "show the native reconstruction version")
    ("scale", boost::program_options::value<int>(), "initial window scale: positive multiple of 640 x 480 (default: fit available screen)");
#ifndef NDEBUG
  options.add_options()
    ("screenshot", boost::program_options::value<std::string>(), "developer use: save the initial 640 x 350 scene as PPM and exit without a window")
    ("seconds", boost::program_options::value<double>()->default_value(0.0), "developer use: close after this many seconds; zero disables the time limit");
#endif // NDEBUG
  boost::program_options::variables_map arguments;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), arguments);
    boost::program_options::notify(arguments);
  } catch(boost::program_options::error const &error) {
    return startup_failure(error.what());
  }
  if(arguments.contains("help")) {
    std::cout << options << std::endl;
    return EXIT_SUCCESS;
  }
  if(arguments.contains("version")) {
    std::cout << "Corncob C++ 0.1.0 (reconstruction in progress)" << std::endl;
    return EXIT_SUCCESS;
  }
  bool const automatic_scale{!arguments.contains("scale")};
  int const scale{automatic_scale ? 1 : arguments["scale"].as<int>()};
  if(scale < 1 || scale > std::numeric_limits<int>::max() / corncob::render::screen_layout::display_width) {
    return startup_failure("--scale must be a positive integer whose window dimensions fit in an int");
  }
#ifndef NDEBUG
  double const seconds{arguments["seconds"].as<double>()};
  if(seconds < 0.0 || !std::isfinite(seconds)) return startup_failure("--seconds must be a finite non-negative number");
#endif // NDEBUG
  corncob::render::framebuffer frame;
  auto const cockpit{corncob::resources::decode_cockpit_mask(corncob::resources::cockpit_asset())};
  corncob::resources::draw_cockpit_preview(cockpit, frame);
#ifndef NDEBUG
  if(arguments.contains("screenshot")) {
    std::ofstream file{arguments["screenshot"].as<std::string>(), std::ios::binary};
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file << "P6\n" << corncob::render::screen_layout::width << ' ' << corncob::render::screen_layout::height << "\n255\n";
    for(auto const &pixel : frame.pixels) {
      file.put(static_cast<char>(pixel.red));
      file.put(static_cast<char>(pixel.green));
      file.put(static_cast<char>(pixel.blue));
    }
    file.close();
    return EXIT_SUCCESS;
  }
#endif // NDEBUG
  glfwSetErrorCallback([](int const code, char const *const message){
    std::cerr << "ERROR: GLFW " << code << ": " << message << std::endl;
  });
  if(!glfwInit()) return startup_failure("GLFW initialisation failed");
  boost::scope::scope_exit terminate_glfw{[]{
    glfwTerminate();
  }};
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  bool const compositor_sizing{automatic_scale && glfwGetPlatform() == GLFW_PLATFORM_WAYLAND};
  glfwWindowHint(GLFW_MAXIMIZED, compositor_sizing ? GLFW_TRUE : GLFW_FALSE);
  std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> const window{
    glfwCreateWindow(corncob::render::screen_layout::display_width * scale, corncob::render::screen_layout::display_height * scale,
                     "Corncob 3D - cockpit resource preview", nullptr, nullptr),
    glfwDestroyWindow,
  };
  if(!window) return startup_failure("cannot create the Corncob window");
  if(automatic_scale && !compositor_sizing) {
    if(auto *monitor{glfwGetPrimaryMonitor()}) {
      int x{0}, y{0}, width{0}, height{0};
      glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
      int left{0}, top{0}, right{0}, bottom{0};
      glfwGetWindowFrameSize(window.get(), &left, &top, &right, &bottom);
      if(width > left + right && height > top + bottom) {
        auto const fitted{corncob::render::fit_viewport(width - left - right, height - top - bottom)};
        glfwSetWindowSize(window.get(), fitted.width, fitted.height);
        glfwSetWindowPos(window.get(), x + left + fitted.x, y + top + fitted.y);
      }
    }
  }
  glfwShowWindow(window.get());
  if(compositor_sizing) {
    int width{0}, height{0};
    glfwGetWindowSize(window.get(), &width, &height);
    glfwRestoreWindow(window.get());
    double const deadline{glfwGetTime() + 1.0};
    while(glfwGetWindowAttrib(window.get(), GLFW_MAXIMIZED) && glfwGetTime() < deadline) glfwWaitEventsTimeout(0.01);
    auto const fitted{corncob::render::fit_viewport(width, height)};
    glfwSetWindowSize(window.get(), fitted.width, fitted.height);
  }
  glfwMakeContextCurrent(window.get());
  glfwSwapInterval(1);
  glfwSetKeyCallback(window.get(), [](GLFWwindow *const window, int const key, int const /*scancode*/, int const action, int const /*modifiers*/){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
  });
  corncob::platform::framebuffer_presenter presenter{*window};
  while(!glfwWindowShouldClose(window.get())) {
    glfwPollEvents();
#ifndef NDEBUG
    if(seconds > 0.0 && glfwGetTime() >= seconds) break;
#endif // NDEBUG
    presenter.present(frame);
    glfwWaitEventsTimeout(1.0 / 60.0);
  }
  return EXIT_SUCCESS;
}

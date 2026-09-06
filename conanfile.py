from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain, CMakeDeps
from conan.tools.files import load


class MothNoiseEditor(ConanFile):
    name = "moth_noised"

    license = "MIT"
    url = "https://github.com/instinkt900/moth_noised"
    description = "A node editor for moth::noise noise graphs"

    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "version.txt", "cmake/*", "src/*", "ipc/*"

    def set_version(self):
        if not self.version:
            self.version = load(self, "version.txt").strip()

    def requirements(self):
        # Only the noise module. The editor is deliberately not a moth_toolkit
        # application: it owns its own window, its own UI stack and its own
        # main loop, and the single thing it shares with the engine is the file
        # format. moth::noise is that format, so it is the whole dependency.
        self.requires("moth_noise/[>=0.1 <1]")

    def configure(self):
        # The UI half of this build brings its own GLFW through Magnum. Without
        # this, moth_core's GLFW backend comes in underneath moth::noise and two
        # copies of GLFW end up in one link.
        self.options["moth_core"].enable_platform = False

    def build_requirements(self):
        # A range rather than a pin: a CI runner image moving to a newer Visual
        # Studio asks Conan for a generator that an older exact CMake does not
        # know, which breaks the build with nothing here having changed. The
        # same reasoning as moth_editor's.
        self.tool_requires("cmake/[>=3.27.0]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeDeps(self).generate()
        CMakeToolchain(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

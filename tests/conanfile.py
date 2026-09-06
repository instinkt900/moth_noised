from conan import ConanFile
from conan.tools.cmake import cmake_layout


class MothNoisedTests(ConanFile):
    name = "moth_noised_tests"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("catch2/3.13.0")
        self.requires("moth_noise/[>=0.1 <1]")

    def configure(self):
        # Same as the editor: no windowing stack under moth::noise. Nothing in
        # these tests needs one, and it keeps the test build to the format.
        self.options["moth_core"].enable_platform = False

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.27.0]")

    def layout(self):
        cmake_layout(self)

import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import cmake_layout, CMakeDeps, CMakeToolchain
from conan.tools.files import copy


class MothNoisedTests(ConanFile):
    name = "moth_noised_tests"
    settings = "os", "compiler", "build_type", "arch"

    def validate(self):
        # C++20, the same as the editor these tests are built from.
        check_min_cppstd(self, 20)

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

    def generate(self):
        CMakeDeps(self).generate()
        tc = CMakeToolchain(self)
        # The DLLs of shared dependencies (FastNoise2 on Windows) are collected
        # into one folder, which the CMake build copies next to the executable.
        runtime_dir = os.path.join(self.generators_folder, "runtime")
        if self.settings.os == "Windows":
            for dep in self.dependencies.host.values():
                for bindir in dep.cpp_info.bindirs:
                    copy(self, "*.dll", bindir, runtime_dir, keep_path=False)
        tc.cache_variables["MOTH_NOISED_RUNTIME_DIR"] = runtime_dir.replace("\\", "/")
        tc.generate()

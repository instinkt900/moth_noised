import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import cmake_layout, CMake, CMakeDeps, CMakeToolchain
from conan.tools.files import copy, load
from conan.tools.system.package_manager import Apt


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

    def validate(self):
        # C++20: the vendored upstream editor uses <bit> (std::popcount), and
        # upstream's CMake asks for cxx_std_20.
        check_min_cppstd(self, 20)

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

    def system_requirements(self):
        # GTK3 for the file dialogs (nativefiledialog-extended, fetched through CPM).
        if self.settings.os == "Linux":
            apt = Apt(self)
            apt.install(["libgtk-3-dev"])

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
        tc = CMakeToolchain(self)
        # moth_noise builds FastNoise2 shared on Windows. The IPC library declares
        # its functions with FastNoise2's export macro, so it has to be linked the
        # same way FastNoise2 is.
        tc.cache_variables["MOTH_NOISED_FASTNOISE_SHARED"] = bool(self.dependencies["fastnoise2"].options.shared)
        # The DLLs of shared dependencies (FastNoise2 on Windows) are collected
        # into one folder, which the CMake build copies next to the executable.
        runtime_dir = os.path.join(self.generators_folder, "runtime")
        if self.settings.os == "Windows":
            for dep in self.dependencies.host.values():
                for bindir in dep.cpp_info.bindirs:
                    copy(self, "*.dll", bindir, runtime_dir, keep_path=False)
        tc.cache_variables["MOTH_NOISED_RUNTIME_DIR"] = runtime_dir.replace("\\", "/")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

{
  pkgs ? import <nixpkgs> { },
}:
pkgs.mkShell {
  packages = with pkgs; [
    clang-tools
    cmake
    freeglut
    glew
    glfw3
    glm
    libGLU
    meson
    ninja
    pkg-config
  ];
}

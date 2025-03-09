{
  pkgs ? import <nixpkgs> { },
}:
pkgs.mkShell {
  packages = with pkgs; [
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

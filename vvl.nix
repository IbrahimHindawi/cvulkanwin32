{
  stdenv,
  fetchFromGitHub,
  cmake,
  pkg-config,
  jq,
  spirv-headers,
  spirv-tools,
  glslang,
  vulkan-headers,
  vulkan-loader,
  vulkan-utility-libraries,
  apple-sdk_15,
}:
stdenv.mkDerivation rec {
  pname = "vulkan-validation-layers";
  version = "1.3.296.0";

  src = fetchFromGitHub {
    owner = "KhronosGroup";
    repo = "Vulkan-ValidationLayers";
    rev = "vulkan-sdk-${version}";
    hash = "sha256-H5AG+PXM3IdCfDqHMdaunRUWRm8QgdS6ZbZLMaOOALk=";
  };

  strictDeps = true;

  nativeBuildInputs = [
    cmake
    pkg-config
    jq
  ];

  buildInputs = [
    spirv-headers
    spirv-tools
    glslang
    vulkan-headers
    vulkan-loader
    vulkan-utility-libraries
    apple-sdk_15
  ];

  cmakeFlags = [
    "-DBUILD_LAYER_SUPPORT_FILES=ON"
    # Hide dev warnings that are useless for packaging
    "-Wno-dev"
    "-DUSE_ROBIN_HOOD_HASHING=OFF"
  ];

  doCheck = false;
  separateDebugInfo = true;

  preFixup = ''
    for f in "$out"/share/vulkan/explicit_layer.d/*.json "$out"/share/vulkan/implicit_layer.d/*.json; do
      jq <"$f" >tmp.json ".layer.library_path = \"$out/lib/\" + .layer.library_path"
      mv tmp.json "$f"
    done
  '';
}

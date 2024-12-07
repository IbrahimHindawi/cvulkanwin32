{
  description = "Particle System in Vulkan & GLFW using C++23";

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-darwin"
        "x86_64-linux"
      ];
      perSystem =
        {
          lib,
          pkgs,
          config,
          ...
        }:
        let
          llvm = pkgs.llvmPackages_19;
          stdenv = llvm.stdenv;

          vulkan-validation-layers = pkgs.callPackage ./vvl.nix { inherit stdenv; };
          glfw = pkgs.glfw.overrideAttrs {
            env.NIX_CFLAGS_COMPILE = toString [
              "-D_GLFW_VULKAN_LIBRARY=\"${lib.getLib pkgs.vulkan-loader}/lib/libvulkan.1.dylib\""
            ];
          };
        in
        {
          packages.particles = stdenv.mkDerivation {
            pname = "particles";
            version = "0.1.0";
            src = ./source;

            outputs = [
              "out"
              "development"
            ];

            nativeBuildInputs = builtins.attrValues {
              inherit (pkgs) makeWrapper shaderc;
            };

            buildInputs =
              builtins.attrValues {
                inherit (pkgs)
                  vulkan-headers
                  vulkan-loader
                  glm
                  ;

                inherit glfw vulkan-validation-layers;
              }
              ++ lib.optionals (stdenv.isDarwin) [
                pkgs.apple-sdk_15
                pkgs.moltenvk
              ];

            FLAGS = [
              "--start-no-unused-arguments"
              "-std=c++23"
              "-stdlib=libc++"
              "-fstrict-enums"
              "-fsanitize=undefined"
              "-fsanitize=address"
              "-fcoroutines"
              "-flto"
              #"-fno-exceptions"
              #"-fno-rtti"
              "-fno-threadsafe-statics"
              "-fno-operator-names"
              "-fno-common"
              "-fvisibility=hidden"
              "-Wall"
              "-Wextra"
              "-Werror"
              "-Wpedantic"
              "-Wconversion"
              "-Wno-missing-field-initializers"
            ];

            preBuild = ''
              mkdir -p pcms
              $CXX  -Wno-reserved-identifier -Wno-reserved-module-identifier \
                    --precompile -o pcms/std.pcm ${llvm.libcxx}/share/libc++/v1/std.cppm $FLAGS
            '';

            buildPhase = ''
              runHook preBuild
              $CXX main.cc -o particles -fprebuilt-module-path=pcms -lglfw -framework Cocoa -framework AppKit -framework Foundation -framework QuartzCore -framework Metal -framework MetalKit -lvulkan -lMoltenVK -MJ particles.o.json $FLAGS
            '';

            installPhase = ''
              install -Dm755 -t $out/bin particles
              install -D -t $development/pcms pcms/*
              install -D -t $development/fragments *.o.json

              wrapProgram $out/bin/particles \
                --suffix VK_LAYER_PATH : ${vulkan-validation-layers}/share/vulkan/explicit_layer.d \
                --suffix VK_ICD_FILENAMES : ${
                  if stdenv.isDarwin then "${pkgs.moltenvk}/share/vulkan/icd.d/MoltenVK_icd.json" else ""
                }
            '';

            meta.mainProgram = "particles";
          };

          devShells.default = pkgs.mkShell.override { inherit stdenv; } {
            packages = [ (pkgs.ccls.override { llvmPackages = llvm; }) ];
            inputsFrom = [ config.packages.particles ];
          };

          apps.ccdb = {
            type = "app";
            program = lib.getExe (
              pkgs.writeShellApplication {
                name = "ccdb";
                runtimeInputs = [ pkgs.gnused ];
                text =
                  let
                    proj = config.packages.particles.development;
                  in
                  ''
                    sed  -e '1s/^/[\n/' -e '$s/,$/\n]/' \
                         -e "s|/private/tmp/nix-build-${proj.name}.drv-0|$(pwd)|g" \
                         -e "s|prebuilt-module-path=pcms|prebuilt-module-path=${proj}/pcms|g" \
                         ${proj}/fragments/*.o.json \
                         > compile_commands.json
                  '';
              }
            );
          };

          packages.default = config.packages.particles;
        };
    };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
}

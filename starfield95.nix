{
  stdenv,
  lib,
}:
stdenv.mkDerivation {
  name = "starfield95";
  src = ./.;
  dontUnpack = true;
  buildPhase = lib.optionalString stdenv.hostPlatform.isDarwin ''
    runHook preBuild
    ${lib.getExe stdenv.cc.cc} -o starfield95 $src/starfield95.c -O2 \
      -framework OpenGL \
      -framework GLUT
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    install -m755 starfield95 $out/bin
    runHook postInstall
  '';
}

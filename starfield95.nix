{
  stdenv,
  lib,
  SDL2,
  SDL2_ttf,
  pkg-config,
  dejavu_fonts,
}:
stdenv.mkDerivation {
  name = "starfield95";
  src = ./.;
  buildInputs = [
    SDL2
    SDL2_ttf
    pkg-config
    dejavu_fonts
  ];
  preBuild = ''
    substituteInPlace starfield95.c \
      --replace "@fontPath@" "${dejavu_fonts}/share/fonts/truetype/DejaVuSans.ttf"
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    install -m755 starfield95 $out/bin
    runHook postInstall
  '';
}

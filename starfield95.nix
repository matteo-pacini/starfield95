{
  stdenv,
  SDL2,
  pkg-config,
}:
stdenv.mkDerivation {
  name = "starfield95";
  src = ./.;
  nativeBuildInputs = [
    pkg-config
  ];
  buildInputs = [
    SDL2
  ];
  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    install -m755 starfield95 $out/bin
    runHook postInstall
  '';
}

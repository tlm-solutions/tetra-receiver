{ stdenv
, pkg-config
, cmake
, gnuradio
, gnuradioPackages
, log4cpp
, mpir
, gmpxx
, cxxopts
}:
let
  osmosdr = gnuradioPackages.osmosdr.overrideAttrs(_oldAttrs: {
    outputs = [ "out" ];
  });
in
stdenv.mkDerivation {
  name = "tetra-receiver";
  version = "0.1.0";

  src = ./..;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ log4cpp mpir gnuradio.unwrapped gnuradio.unwrapped.boost.dev gmpxx.dev gnuradio.unwrapped.volk osmosdr cxxopts ];

  cmakeFlags = [ "-DCMAKE_PREFIX_PATH=${osmosdr}/lib/cmake/osmosdr" ];
}

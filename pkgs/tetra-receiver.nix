{ stdenv
, pkg-config
, cmake
, gnuradio
, gnuradioPackages
, log4cpp
, mpir
, gmp
, gmpxx
, thrift
, hackrf
, rtl-sdr
, fftwFloat
, patchelf
}:
let
  osmosdr = gnuradioPackages.osmosdr.overrideAttrs(oldAttrs: {
    outputs = [ "out" ];
  });
in
stdenv.mkDerivation {
  name = "tetra-receiver";
  version = "0.1.0";

  src = ./..;

  nativeBuildInputs = [ cmake pkg-config gnuradio.unwrapped ];
  buildInputs = [ log4cpp mpir gnuradio.unwrapped.boost.dev gmpxx.dev gnuradio.unwrapped.volk osmosdr ];

  cmakeFlags = [ "-DOSMOSDR_DIR=${osmosdr}" "-DCMAKE_PREFIX_PATH=${osmosdr}/lib/cmake/osmosdr" ];
}

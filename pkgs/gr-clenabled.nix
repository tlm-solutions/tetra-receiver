{ pkg-config
, cmake
, gnuradio
, log4cpp
, mpir
, gmp
, fftwFloat
, rocmPackages
, opencl-clhpp
, clfft
, cppunit
, fetchFromGitHub
, stdenv
}:

stdenv.mkDerivation rec {
  name = "gr-clenabled";
  version = "maint-3.8";
  src = fetchFromGitHub {
    owner = "ghostop14";
    repo = name;
    rev = version;
    sha256 = "sha256-n8jDcjQfopo/9XZNrquGTO4dp3ErXnwH+ABgmbTawiA=";
  };
  buildInputs = [ gnuradio log4cpp mpir gmp gnuradio.boost gnuradio.volk fftwFloat rocmPackages.clr opencl-clhpp clfft cppunit ];
  nativeBuildInputs = [ cmake pkg-config ];
  cmakeFlags = [ "-DENABLE_PYTHON=OFF" ];
  # outputs = [ "out" ];
}
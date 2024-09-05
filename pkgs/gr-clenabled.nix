{ pkg-config
, cmake
, gnuradio3_8
, log4cpp
, mpir
, gmp
, fftwFloat
, opencl-headers
, ocl-icd
, opencl-clhpp
, clfft
, cppunit
, python3
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
  buildInputs = [ gnuradio3_8.unwrapped log4cpp mpir gmp gnuradio3_8.unwrapped.boost gnuradio3_8.unwrapped.volk python3 fftwFloat ocl-icd opencl-headers opencl-clhpp clfft cppunit ];
  nativeBuildInputs = [ cmake pkg-config ];
  cmakeFlags = [ "-DENABLE_PYTHON=OFF" ];
  patches = [ ./gr-clenabled-use-correct-opencl.patch ];
}

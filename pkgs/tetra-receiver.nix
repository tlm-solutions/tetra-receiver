{ stdenv
, pkg-config
, cmake
, gnuradio
, gnuradioPackages
, log4cpp
, mpir
, gmpxx
, cxxopts
, gtest
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
  buildInputs = [ log4cpp mpir gnuradio.unwrapped gnuradio.unwrapped.boost.dev gmpxx.dev gnuradio.unwrapped.volk osmosdr cxxopts gtest ];

  cmakeFlags = [ "-DCMAKE_PREFIX_PATH=${osmosdr}/lib/cmake/osmosdr" ];

	installPhase = ''
		ls -alh ./
		mkdir -p $out/bin
		cp ./test/unit_tests $out/bin/test
		cp ./tetra-receiver $out/bin/tetra-receiver
		#cp ./build/tetra-receiver $out/bin
		#cp ./build/test $out/bin
	'';
}

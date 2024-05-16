{ pkg-config
, cmake
, gnuradio
, gnuradioPackages
, log4cpp
, mpir
, gmpxx
, cxxopts
, toml11
, fetchFromGitHub
, stdenv
, gtest
, prometheus-cpp
, zlib
, curlFull
, glibc
}:
let
  toml11 = stdenv.mkDerivation {
		pname = "toml11";
		version = "3.8.1";

		src = fetchFromGitHub {
			owner = "ToruNiina";
			repo = "toml11";
			rev = "v3.8.1";
			hash = "sha256-XgpsCv38J9k8Tsq71UdZpuhaVHK3/60IQycs9CeLssA=";
		};
		phases = ["unpackPhase" "installPhase"];

		nativeBuildInputs = [
			cmake
		];

		installPhase = ''
			mkdir -p $out/include

			cp -r ./toml.hpp $out/include/
			cp -r ./toml $out/include 
			cp -r cmake $out/
		'';
	};
  osmosdr = gnuradioPackages.osmosdr.overrideAttrs(_oldAttrs: {
    outputs = [ "out" ];
  });
in
stdenv.mkDerivation {
  name = "tetra-receiver";
  version = "0.1.0";

  src = ./..;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [
    log4cpp
    mpir
    gnuradio.unwrapped
    gnuradio.unwrapped.boost.dev
    gmpxx.dev
    gnuradio.unwrapped.volk
    osmosdr
    cxxopts
    toml11
    gtest
    prometheus-cpp
    zlib
    curlFull
  ];

  cmakeFlags = [ "-DCMAKE_PREFIX_PATH=${osmosdr}/lib/cmake/osmosdr" ];

	installPhase = ''
		mkdir -p $out/bin
    cp ./test/unit_tests $out/bin/test
    cp ./tetra-receiver $out/bin/tetra-receiver
	'';
}

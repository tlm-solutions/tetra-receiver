{ pkg-config
, cmake
, gnuradio3_8
, gnuradio3_8Packages
, log4cpp
, mpir
, gmpxx
, cxxopts
, fetchFromGitHub
, stdenv
, gtest
, prometheus-cpp
, zlib
, glibc
, curlFull
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
  osmosdr = gnuradio3_8Packages.osmosdr.overrideAttrs(_oldAttrs: {
    outputs = [ "out" ];
  });
in
stdenv.mkDerivation rec {
  name = "tetra-receiver";
  version = "0.1.0";

  src = ./..;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [
    log4cpp
    mpir
    gnuradio3_8.unwrapped
    gnuradio3_8.unwrapped.boost.dev
    gmpxx.dev
    gnuradio3_8.unwrapped.volk
    osmosdr
    cxxopts
    toml11
    gtest
    prometheus-cpp
    zlib
    curlFull
    #glibc
  ];
  preConfigure = ''
    echo "-DCMAKE_PREFIX_PATH=${osmosdr}/lib/cmake/osmosdr"
  '';

  cmakeFlags = [ "-DCMAKE_PREFIX_PATH=${osmosdr}/lib/cmake/osmosdr" ];

	installPhase = ''
		mkdir -p $out/bin
    cp ./test/unit_tests $out/bin/test
    cp ./tetra-receiver $out/bin/tetra-receiver
	'';
}

{ pkgs, config, lib, ... }:
let cfg = config.services.tetra-receiver;
in {
  options.services.tetra-receiver = with lib; {
    enable = mkEnableOption "tetra-receiver";
    rfGain = mkOption {
      type = types.int;
      default = 10;
      description = "	RF gain (default: 10)\n";
    };
    ifGain = mkOption {
      type = types.int;
      default = 10;
      description = "	IF gain (default: 10)\n";
    };
    bbGain = mkOption {
      type = types.int;
      default = 10;
      description = "	BB gain (default: 10)\n";
    };
    deviceString = mkOption {
      type = types.str;
      default = "";
      description =
        "	additional device arguments for osmosdr, see https://projects.osmocom.org/projects/gr-osmosdr/wiki/GrOsmoSDR (default: \"\")\n";
    };
    centerFrequency = mkOption {
      type = types.int;
      default = 0;
      description = "	Center frequency of the SDR (default: 0)\n";
    };
    offsets = mkOption {
      type = types.listOf types.int;
      default = [ ];
      description = "	Offsets of the TETRA streams\n";
    };
    sampRate = mkOption {
      type = types.int;
      default = 1000000;
      description = "	Sample rate of the sdr (default: 1000000)\n";
    };
    udpStart = mkOption {
      type = types.int;
      default = 42000;
      description =
        "	Start UDP port. Each stream gets its own UDP port, starting at udp-start (default: 42000)\n";
    };
    user = mkOption {
      type = types.str;
      default = "tetra-receiver";
      description = ''
        systemd user
      '';
    };
    group = mkOption {
      type = types.str;
      default = "tetra-receiver";
      description = ''
        group of systemd user
      '';
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services."tetra-receiver" = {
      enable = true;
      wantedBy = [ "multi-user.target" ];

      script = ''
        exec ${pkgs.tetra-receiver}/bin/tetra-receiver --rf ${toString cfg.rfGain} --if ${toString cfg.ifGain} --bb ${toString cfg.bbGain} --device-string "${cfg.deviceString}" --offsets ${
          lib.concatMapStringsSep "," toString cfg.offsets
        } --center-frequency ${toString cfg.centerFrequency} --samp-rate ${toString cfg.sampRate} --udp-start ${toString cfg.udpStart} &
      '';

      serviceConfig = {
        Type = "forking";
        User = cfg.user;
        Restart = "always";
      };
    };

    security.wrappers.tetra-receiver = {
      owner = cfg.user;
      group = "users";
      capabilities = "cap_sys_nice+eip";
      source = "${pkgs.tetra-receiver}/bin/tetra-receiver";
    };

    # user accounts for systemd units
    users.users."${cfg.user}" = {
      name = cfg.user;
      description = "This users runs tetra-receiver";
      isNormalUser = true;
      group = cfg.group;
      extraGroups = [ "plugdev" ];
    };
  };
}

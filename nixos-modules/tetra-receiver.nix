{ pkgs, config, lib, ... }:
let cfg = config.services.tetra-receiver;
in {
  options.services.tetra-receiver = with lib; {
    enable = mkEnableOption "tetra-receiver";
    configFile = mkOption {
      type = types.str;
      default = "";
      description = "The contents of the toml config file.";
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

      script = let
        configFile = pkgs.writeTextFile {
          name = "tetra-receiver-config.toml";
          text = cfg.configFile;
        };
      in ''
        exec ${pkgs.expect}/bin/unbuffer ${pkgs.tetra-receiver}/bin/tetra-receiver --config-file ${configFile} &
      '';

      serviceConfig = {
        Type = "forking";
        User = cfg.user;
        Restart = "always";
      };
    };

    security.wrappers.tetra-receiver = {
      owner = cfg.user;
      group = cfg.group;
      capabilities = "cap_sys_nice+eip";
      source = "${pkgs.tetra-receiver}/bin/tetra-receiver";
    };

    # user accounts for systemd units
    users.groups."${cfg.group}" = {};

    users.users."${cfg.user}" = {
      name = cfg.user;
      description = "This users runs tetra-receiver";
      isNormalUser = true;
      group = cfg.group;
      extraGroups = [ "plugdev" ];
    };
  };
}

M_PLUGIN_DIR="${0:A:h}"

_load_configs(){

        local config_dir="$M_PLUGIN_DIR/configs"
        if [[ ! -d "$config_dir" ]]; then
               echo "[work-flow] Error : configs directory not found: $config_dir" 
               return 1
        fi

        # look up all .zsh files include sub directory
        local config_files=("$config_dir"/**/*.zsh(.))

        if (( ${#config_files} == 0 )); then
                echo "[work-flow] No config files found in $config_dir"
                return 2
        fi

        for file in "${(o)config_files[@]}";do
                if [[ -r "$file" ]]; then
                        source "$file"
                fi
        done
}

_load_configs
start_tmux

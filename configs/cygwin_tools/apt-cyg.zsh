if [[ "$(uname)" =~ "CYGWIN_*" ]]; then
        M_DIR="${0:A:h}"
        alias apt=$M_DIR/apt-cyg
fi

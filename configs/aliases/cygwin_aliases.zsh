local M_ALIASES_DIR="${0:A:h}"
if [[ "$(uname)" =~ "CYGWIN_*" ]]; then
        source ${M_ALIASES_DIR}/base_aliases.sh
        export PATH=/sbin:$PATH
fi

local M_COMPLETION_DIR="${0:A:h}"
if [[ "$(uname)" =~ "CYGWIN_*" ]]; then
        source ${M_COMPLETION_DIR}/base_completion.sh
fi

TMUX_CONFIG_PATH="${0:A:h}/.tmux.conf"
set_log_level info

start_tmux() {
        log_enter
        local SESSION_MAIN="main"
        if [ -n "$TMUX" ]; then
               tmux switch-client -t $SESSION_MAIN 
               log_debug "aleady exists tmux session"
       else


        tmux has-session -t $SESSION_MAIN 2>/dev/null
        if [ $? != 0 ]; then
            tmux new-session -d -s $SESSION_MAIN -n logToAnalyze
            tmux new-window -t $SESSION_MAIN:1 -n SourceCode
            tmux new-window -t $SESSION_MAIN:2 -n Gtd
            tmux source-file $TMUX_CONFIG_PATH
        fi

        tmux attach-session -t $SESSION_MAIN

        fi

}

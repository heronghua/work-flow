start_tmux() {
        #echo "[start_tmux]"
        local SESSION_MAIN="main"
        if [ -n "$TMUX" ]; then
               tmux switch-client -t $SESSION_MAIN 
               #echo "aleady exists tmux session"
       else


        tmux has-session -t $SESSION_MAIN 2>/dev/null
        if [ $? != 0 ]; then
            tmux new-session -d -s $SESSION_MAIN -n log2Analyze
            tmux new-window -t $SESSION_MAIN:1 -n SourceCode
            tmux new-window -t $SESSION_MAIN:2 -n Gtd
        fi

        tmux attach-session -t $SESSION_MAIN

        fi

}

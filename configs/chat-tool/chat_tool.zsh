M_CHAT_TOOL_PLUGIN_DIR="${0:A:h}"

chat_tool_server_start() {
        python "$M_CHAT_TOOL_PLUGIN_DIR"/server.py
}

chat_tool_client_start(){
        python "$M_CHAT_TOOL_PLUGIN_DIR"/client.py
}

// 创建右键菜单
chrome.contextMenus.create({
  id: "open-path-with-explorer",
  title: "用文件浏览器打开",
  contexts: ["selection"]   // 仅当选中文本时显示
});

// 监听菜单点击
chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId === "open-path-with-explorer") {
    const selectedText = info.selectionText.trim();
    // 验证选中的文本是否是一个合法的路径（简单验证）
    if (selectedText) {
      // 发送给本地应用
      sendPathToNativeApp(selectedText);
    }
  }
});

function sendPathToNativeApp(path) {
  // 本地应用的Native Messaging host名称
  const hostName = "com.example.open_path_host";

  // 尝试连接Native Host
  try {
    const port = chrome.runtime.connectNative(hostName);
    port.postMessage({ path: path });
    port.onMessage.addListener((response) => {
      console.log("Received response: " + response);
    });
    port.onDisconnect.addListener(() => {
      console.log("Disconnected");
    });
  } catch (e) {
    console.error("Error connecting to native application:", e);
    // 可以在这里通知用户，比如需要安装本地应用
  }
}
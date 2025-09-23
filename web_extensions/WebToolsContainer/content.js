// 在页面上注入代码的功能
chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
    if (request.action === "injectCode") {
        // 将HTML代码注入当前页面
        const div = document.createElement('div');
        div.innerHTML = request.code;
        document.body.appendChild(div);
        sendResponse({success: true});
    }
});
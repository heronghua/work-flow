// 跨浏览器兼容
var browser = browser || chrome;

// ---------- 工具函数：打开首页 ----------
function openToolsPage() {
    var url = browser.runtime.getURL('index.html');
    console.log('[Tools] Opening URL:', url);
    browser.tabs.create({ url: url }).then(function(tab) {
        console.log('[Tools] Tab created with ID:', tab.id);
    }).catch(function(err) {
        console.error('[Tools] Failed to create tab:', err);
        if (browser.notifications) {
            browser.notifications.create({
                type: 'basic',
                iconUrl: 'icon.png',
                title: '错误',
                message: '无法打开工具页，请检查扩展权限。'
            });
        }
    });
}

// ---------- 工具栏按钮点击 ----------
browser.browserAction.onClicked.addListener(function(tab) {
    console.log('[Tools] Browser action clicked');
    openToolsPage();
});

// ---------- Omnibox 地址栏触发 (安全检测) ----------
if (browser.omnibox && typeof browser.omnibox.onInputEnter === 'object' && typeof browser.omnibox.onInputChanged === 'object') {
    // 输入建议
    browser.omnibox.onInputChanged.addListener(function(text, suggest) {
        console.log('[Omnibox] onInputChanged:', text);
        suggest([{ content: text, description: '打开工具集首页' }]);
    });

    // 用户按回车
    browser.omnibox.onInputEnter.addListener(function(text, disposition) {
        console.log('[Omnibox] onInputEnter triggered with:', text);
        openToolsPage();
    });
    console.log('[Tools] Omnibox API registered successfully.');
} else {
    console.warn('[Tools] Omnibox API not available. Address bar launch will not work.');
}

console.log('[Tools] Background script loaded.');
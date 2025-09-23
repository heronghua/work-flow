// 工具数据模型
let tools = [];

// 初始化
document.addEventListener('DOMContentLoaded', function() {
    loadTools();
    document.getElementById('add-tool-btn').addEventListener('click', openAddToolModal);
    document.querySelector('.search-box').addEventListener('input', filterTools);
});

// 从存储加载工具
function loadTools() {
    chrome.storage.local.get(['htmlTools'], function(result) {
        tools = result.htmlTools || [];
        renderTools();
    });
}

// 渲染工具列表
function renderTools(filteredTools = null) {
    const toolsList = document.getElementById('tools-list');
    const toolsToRender = filteredTools || tools;
    
    if (toolsToRender.length === 0) {
        toolsList.innerHTML = '<div class="empty-state">暂无工具，点击"添加新工具"开始收集</div>';
        return;
    }
    
    toolsList.innerHTML = '';
    toolsToRender.forEach(tool => {
        const toolElement = document.createElement('div');
        toolElement.className = 'tool-item';
        toolElement.innerHTML = `
            <div class="tool-header">
                <span class="tool-name">${tool.name}</span>
                <span class="tool-category">${tool.category}</span>
            </div>
            <div class="tool-actions">
                <button class="btn btn-primary copy-btn" data-id="${tool.id}">复制代码</button>
                <button class="btn btn-secondary edit-btn" data-id="${tool.id}">编辑</button>
            </div>
        `;
        toolsList.appendChild(toolElement);
    });
    
    // 添加事件监听
    document.querySelectorAll('.copy-btn').forEach(btn => {
        btn.addEventListener('click', copyToolCode);
    });
    
    document.querySelectorAll('.edit-btn').forEach(btn => {
        btn.addEventListener('click', editTool);
    });
}

// 复制工具代码
function copyToolCode(e) {
    const toolId = e.target.getAttribute('data-id');
    const tool = tools.find(t => t.id === toolId);
    
    if (tool) {
        navigator.clipboard.writeText(tool.code)
            .then(() => {
                showNotification('代码已复制到剪贴板');
            })
            .catch(err => {
                console.error('复制失败: ', err);
                showNotification('复制失败，请手动复制');
            });
    }
}

// 编辑工具
function editTool(e) {
    const toolId = e.target.getAttribute('data-id');
    // 打开编辑界面
    chrome.tabs.create({
        url: chrome.runtime.getURL('editor.html') + '?id=' + toolId
    });
}

// 过滤工具
function filterTools(e) {
    const searchTerm = e.target.value.toLowerCase();
    const filteredTools = tools.filter(tool => 
        tool.name.toLowerCase().includes(searchTerm) || 
        tool.category.toLowerCase().includes(searchTerm) ||
        tool.description.toLowerCase().includes(searchTerm)
    );
    renderTools(filteredTools);
}

// 打开添加工具模态框
function openAddToolModal() {
    chrome.tabs.create({
        url: chrome.runtime.getURL('editor.html')
    });
}

// 显示通知
function showNotification(message) {
    // 简单的通知实现
    const notification = document.createElement('div');
    notification.textContent = message;
    notification.style.cssText = `
        position: fixed;
        top: 10px;
        right: 10px;
        background: #4CAF50;
        color: white;
        padding: 10px;
        border-radius: 5px;
        z-index: 1000;
    `;
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.remove();
    }, 2000);
}
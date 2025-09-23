document.addEventListener('DOMContentLoaded', function() {
    const urlParams = new URLSearchParams(window.location.search);
    const toolId = urlParams.get('id');
    
    if (toolId) {
        // 编辑现有工具
        document.getElementById('editor-title').textContent = '编辑HTML工具';
        loadToolForEditing(toolId);
    }
    
    document.getElementById('tool-form').addEventListener('submit', saveTool);
    document.getElementById('cancel-btn').addEventListener('click', function() {
        window.close();
    });
    
    // 实时预览
    document.getElementById('tool-code').addEventListener('input', updatePreview);
});

// 加载工具数据用于编辑
function loadToolForEditing(toolId) {
    chrome.storage.local.get(['htmlTools'], function(result) {
        const tools = result.htmlTools || [];
        const tool = tools.find(t => t.id === toolId);
        
        if (tool) {
            document.getElementById('tool-name').value = tool.name;
            document.getElementById('tool-category').value = tool.category;
            document.getElementById('tool-description').value = tool.description || '';
            document.getElementById('tool-code').value = tool.code;
            updatePreview();
        }
    });
}

// 更新预览
function updatePreview() {
    const code = document.getElementById('tool-code').value;
    document.getElementById('preview').innerHTML = code;
}

// 保存工具
function saveTool(e) {
    e.preventDefault();
    
    const urlParams = new URLSearchParams(window.location.search);
    const toolId = urlParams.get('id');
    
    const toolData = {
        name: document.getElementById('tool-name').value,
        category: document.getElementById('tool-category').value,
        description: document.getElementById('tool-description').value,
        code: document.getElementById('tool-code').value,
        id: toolId || Date.now().toString()
    };
    
    chrome.storage.local.get(['htmlTools'], function(result) {
        let tools = result.htmlTools || [];
        
        if (toolId) {
            // 更新现有工具
            const index = tools.findIndex(t => t.id === toolId);
            if (index !== -1) {
                tools[index] = toolData;
            }
        } else {
            // 添加新工具
            tools.push(toolData);
        }
        
        chrome.storage.local.set({htmlTools: tools}, function() {
            alert('工具保存成功！');
            window.close();
        });
    });
}
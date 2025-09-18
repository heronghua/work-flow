折线图解析工具
一个简洁、可扩展的折线图解析工具，支持从文件加载数据和自定义解析器。

功能特点
🎯 简洁界面：仅包含必要的文件选择和图表显示功能

📁 文件加载：支持从文件加载数据和解析器脚本

🔧 可扩展性：支持自定义解析器，适应不同数据格式

📊 交互式图表：使用Chart.js生成美观的交互式折线图

📱 响应式设计：适配不同屏幕尺寸

使用方法
1. 准备文件
首先，您需要准备两个文件：

数据文件（.txt、.data或.csv格式）：包含要可视化的数据

解析器脚本（.js格式）：包含解析数据的JavaScript函数

2. 使用工具
打开index.html文件

点击"选择数据文件"按钮，选择您的数据文件

点击"选择解析器脚本"按钮，选择您的解析器脚本文件

当两个文件都选择后，"解析数据并生成图表"按钮将变为可用状态

点击"解析数据并生成图表"按钮

查看右侧生成的交互式折线图

解析器开发指南
基本结构
解析器脚本必须导出一个默认函数，该函数接收数据文件内容作为参数，并返回一个包含图表数据的对象。

javascript
// 解析器示例
export default function myParser(text) {
    // 解析逻辑
    // ...
    
    return {
        labels: ['标签1', '标签2', '标签3'], // X轴标签
        datasets: [
            {
                label: '数据集1',           // 数据系列名称
                data: [10, 20, 30],        // 数据值
                borderColor: 'rgb(75, 192, 192)', // 线条颜色
                backgroundColor: 'rgba(75, 192, 192, 0.2)', // 填充颜色
                tension: 0.3               // 线条曲率
            }
            // 可以添加更多数据集...
        ]
    };
}
数据格式示例
示例数据文件 (sales.data)
text
月份:1,2,3,4,5,6,7,8,9,10,11,12
销售额:12000,15000,13000,20000,18000,22000,24000,21000,23000,25000,28000,30000
利润:3000,4000,3500,5000,4500,5500,6000,5200,5800,6200,7000,7500
对应解析器 (salesParser.js)
javascript
export default function salesParser(text) {
    const lines = text.split('\n').filter(line => line.trim() !== '');
    const result = {};
    
    // 解析每一行
    for (const line of lines) {
        if (!line.includes(':')) continue;
        
        const [key, values] = line.split(':');
        const trimmedKey = key.trim();
        if (!trimmedKey) continue;
        
        // 解析值
        const parsedValues = values.split(',')
            .map(v => v.trim())
            .filter(v => v !== '')
            .map(v => Number(v));
        
        result[trimmedKey] = parsedValues;
    }
    
    // 确定X轴数据
    let labels = [];
    if (result['月份']) {
        labels = result['月份'].map(m => `${m}月`);
        delete result['月份'];
    } else {
        // 如果没有月份数据，使用索引
        const firstKey = Object.keys(result)[0];
        labels = result[firstKey].map((_, index) => index + 1);
    }
    
    // 准备数据集
    const datasets = [];
    const colors = [
        'rgb(75, 192, 192)',
        'rgb(255, 99, 132)',
        'rgb(54, 162, 235)'
    ];
    
    let colorIndex = 0;
    for (const [key, values] of Object.entries(result)) {
        datasets.push({
            label: key,
            data: values,
            borderColor: colors[colorIndex % colors.length],
            backgroundColor: colors[colorIndex % colors.length] + '20',
            tension: 0.3,
            fill: false
        });
        colorIndex++;
    }
    
    return { labels, datasets };
}
高级用法
自定义图表样式
您可以在解析器中自定义图表的各种样式：

javascript
// 在返回的数据集中添加更多样式选项
{
    label: '数据集名称',
    data: [10, 20, 30],
    borderColor: 'rgb(75, 192, 192)',
    backgroundColor: 'rgba(75, 192, 192, 0.2)',
    borderWidth: 2,           // 线条宽度
    pointRadius: 5,           // 数据点半径
    pointHoverRadius: 8,      // 悬停时数据点半径
    pointBackgroundColor: '#fff', // 数据点背景颜色
    tension: 0.4,             // 线条曲率 (0-1)
    fill: true,               // 是否填充区域
    stepped: false,           // 是否使用阶梯线
    // 更多选项请参考Chart.js文档
}
处理不同数据格式
您可以创建不同的解析器来处理各种数据格式：

CSV格式：使用逗号分隔的值

TSV格式：使用制表符分隔的值

JSON格式：直接解析JSON数据

自定义格式：根据您的需求定制

示例文件
本工具附带以下示例文件：

sales.data - 销售数据示例

basicParser.js - 基本解析器示例

您可以使用这些示例文件测试工具功能。

技术细节
使用Chart.js 3.x渲染图表

使用ES6模块动态加载解析器

纯前端实现，无需服务器支持

支持现代浏览器（Chrome、Firefox、Safari、Edge）

故障排除
常见问题
解析器未加载：确保解析器脚本使用正确的导出格式（export default）

图表未显示：检查浏览器控制台是否有错误信息

数据格式错误：确认数据文件格式与解析器期望的格式匹配

获取帮助
如果您遇到问题，请：

检查浏览器控制台中的错误信息

确保您的解析器函数正确返回{labels, datasets}对象

参考Chart.js文档了解图表配置选项

许可证
MIT License - 您可以自由使用、修改和分发此工具。

更新日志
v1.0.0 (2023-10-01)
初始版本发布

支持文件加载和数据解析

基本图表显示功能

如有问题或建议，请提交Issue或Pull Request。


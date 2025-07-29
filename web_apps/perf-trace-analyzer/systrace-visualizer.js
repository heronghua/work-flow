// systrace-visualizer.js

document.addEventListener('DOMContentLoaded', function() {
    // 获取DOM元素
    const dutFileInput = document.getElementById('dut-file');
    const refFileInput = document.getElementById('ref-file');
    const tagsInput = document.getElementById('tags');
    const analyzeBtn = document.getElementById('analyze-btn');
    const lineChartBtn = document.getElementById('line-chart-btn');
    const scatterChartBtn = document.getElementById('scatter-chart-btn');
    const resetBtn = document.getElementById('reset-btn');
    const chartCanvas = document.getElementById('trace-chart');
    
    // 图表变量
    let traceChart = null;
    let chartType = 'line';
    let dutData = null;
    let refData = null;
    let currentTags = [];
    
    // 颜色配置
    const dutColors = ['#3498db', '#2980b9', '#1f618d'];
    const refColors = ['#2ecc71', '#27ae60', '#219653'];
    
    // 初始化按钮状态
    lineChartBtn.classList.add('btn-secondary');
    
    // 事件监听
    analyzeBtn.addEventListener('click', analyzeFiles);
    lineChartBtn.addEventListener('click', () => setChartType('line'));
    scatterChartBtn.addEventListener('click', () => setChartType('scatter'));
    resetBtn.addEventListener('click', resetApp);
    
    // 设置图表类型
    function setChartType(type) {
        chartType = type;
        
        // 更新按钮样式
        lineChartBtn.classList.remove('btn-secondary');
        scatterChartBtn.classList.remove('btn-secondary');
        
        if (type === 'line') {
            lineChartBtn.classList.add('btn-secondary');
        } else {
            scatterChartBtn.classList.add('btn-secondary');
        }
        
        // 如果已有数据，重新渲染图表
        if (dutData || refData) {
            renderChart();
        }
    }
    
    // 分析文件
    function analyzeFiles() {
        const dutFile = dutFileInput.files[0];
        const refFile = refFileInput.files[0];
        const tags = tagsInput.value.split(',').map(tag => tag.trim()).filter(tag => tag);
        
        if (!dutFile && !refFile) {
            alert('请至少上传一个Systrace文件');
            return;
        }
        
        if (!tags.length) {
            alert('请输入要分析的tag列表');
            return;
        }
        
        currentTags = tags;
        
        // 重置数据
        dutData = null;
        refData = null;
        
        // 读取并解析文件
        const promises = [];
        
        if (dutFile) {
            promises.push(readFile(dutFile).then(content => {
                dutData = parseSystrace(content, tags);
            }));
        }
        
        if (refFile) {
            promises.push(readFile(refFile).then(content => {
                refData = parseSystrace(content, tags);
            }));
        }
        
        // 所有文件解析完成后渲染图表
        Promise.all(promises).then(() => {
            renderChart();
        }).catch(error => {
            console.error('文件解析错误:', error);
            alert('解析文件时出错: ' + error.message);
        });
    }
    
    // 读取文件内容
    function readFile(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (event) => resolve(event.target.result);
            reader.onerror = (error) => reject(error);
            reader.readAsText(file);
        });
    }
    
    // 解析Systrace文件
    function parseSystrace(content, tags) {
        // 在实际应用中，这里应该包含完整的解析逻辑
        // 为演示目的，我们生成模拟数据
        
        const result = {};
        
        tags.forEach(tag => {
            result[tag] = generateMockTraceData();
        });
        
        return result;
    }
    
    // 生成模拟的trace数据
    function generateMockTraceData() {
        const data = [];
        let time = 0;
        let value = 30 + Math.random() * 50;
        
        for (let i = 0; i < 50; i++) {
            time += 10 + Math.random() * 20;
            value = Math.max(10, Math.min(100, value + (Math.random() - 0.5) * 15));
            data.push({ time, value });
        }
        
        return data;
    }
    
    // 渲染图表
    function renderChart() {
        // 销毁现有图表
        if (traceChart) {
            traceChart.destroy();
        }
        
        const datasets = [];
        const displayOption = document.querySelector('input[name="data-display"]:checked').id;
        
        // 添加DUT数据
        if (dutData && displayOption !== 'show-ref') {
            currentTags.forEach((tag, index) => {
                if (dutData[tag]) {
                    datasets.push({
                        label: `DUT - ${tag}`,
                        data: dutData[tag].map(point => ({ x: point.time, y: point.value })),
                        borderColor: dutColors[index % dutColors.length],
                        backgroundColor: 'transparent',
                        borderWidth: 2,
                        pointRadius: chartType === 'line' ? 3 : 5,
                        tension: 0.1,
                        showLine: chartType === 'line'
                    });
                }
            });
        }
        
        // 添加REF数据
        if (refData && displayOption !== 'show-dut') {
            currentTags.forEach((tag, index) => {
                if (refData[tag]) {
                    datasets.push({
                        label: `REF - ${tag}`,
                        data: refData[tag].map(point => ({ x: point.time, y: point.value })),
                        borderColor: refColors[index % refColors.length],
                        backgroundColor: 'transparent',
                        borderWidth: 2,
                        pointRadius: chartType === 'line' ? 3 : 5,
                        borderDash: [5, 5],
                        tension: 0.1,
                        showLine: chartType === 'line'
                    });
                }
            });
        }
        
        // 创建新图表
        const ctx = chartCanvas.getContext('2d');
        traceChart = new Chart(ctx, {
            type: chartType,
            data: {
                datasets: datasets
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        title: {
                            display: true,
                            text: '时间 (ms)'
                        },
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: '性能指标'
                        },
                        min: 0,
                        max: 100,
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        }
                    }
                },
                plugins: {
                    legend: {
                        position: 'top',
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false
                    }
                },
                hover: {
                    mode: 'nearest',
                    intersect: true
                }
            }
        });
    }
    
    // 重置应用
    function resetApp() {
        dutFileInput.value = '';
        refFileInput.value = '';
        tagsInput.value = '';
        
        if (traceChart) {
            traceChart.destroy();
            traceChart = null;
        }
        
        dutData = null;
        refData = null;
        currentTags = [];
        
        // 重置图表类型
        setChartType('line');
        
        // 重置显示选项
        document.getElementById('show-both').checked = true;
    }
    
    // 初始化
    resetApp();
});
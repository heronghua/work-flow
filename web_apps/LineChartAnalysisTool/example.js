// 基本解析器示例
export default function basicParser(text) {
    const lines = text.split('\n').filter(line => line.trim() !== '');
    const result = {};
    
    // 解析每一行
    for (const line of lines) {
        if (!line.includes(':')) {
            continue;
        }
        
        const [key, values] = line.split(':');
        const trimmedKey = key.trim();
        
        if (!trimmedKey) {
            continue;
        }
        
        // 解析值
        const parsedValues = values.split(',')
            .map(v => v.trim())
            .filter(v => v !== '')
            .map(v => {
                // 尝试转换为数字
                const num = Number(v);
                return isNaN(num) ? v : num;
            });
        
        result[trimmedKey] = parsedValues;
    }
    
    // 检查数据有效性
    if (Object.keys(result).length === 0) {
        throw new Error('没有解析到有效数据');
    }
    
    // 确定X轴数据
    let labels = [];
    if (result['月份']) {
        labels = result['月份'];
        delete result['月份'];
    } else if (result['x']) {
        labels = result['x'];
        delete result['x'];
    } else {
        // 如果没有指定X轴数据，使用索引作为X轴
        const firstKey = Object.keys(result)[0];
        labels = result[firstKey].map((_, index) => index + 1);
    }
    
    // 准备数据集
    const datasets = [];
    const colors = [
        'rgb(75, 192, 192)',
        'rgb(255, 99, 132)',
        'rgb(54, 162, 235)',
        'rgb(255, 205, 86)',
        'rgb(153, 102, 255)',
        'rgb(255, 159, 64)',
        'rgb(201, 203, 207)'
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
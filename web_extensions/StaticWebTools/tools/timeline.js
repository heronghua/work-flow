(function() {
    // ----- DOM 引用 -----
    var fileInput = document.getElementById('fileInput');
    var logInput = document.getElementById('logInput');
    var parseBtn = document.getElementById('parseBtn');
    var exportBtn = document.getElementById('exportBtn');
    var clearBtn = document.getElementById('clearBtn');
    var chartContainer = document.getElementById('chartContainer');
    var noDataMsg = document.getElementById('noDataMessage');
    var statsArea = document.getElementById('statsArea');
    var eventCountEl = document.getElementById('eventCount');
    var appCountEl = document.getElementById('appCount');
    var timeSpanEl = document.getElementById('timeSpan');
    var anrCountEl = document.getElementById('anrCount');
    var crashCountEl = document.getElementById('crashCount');

    // ----- 全局数据 -----
    var intervals = [];            // 区间 [{app, startTs, endTs}]
    var anrEvents = [];            // [{ts, app, pid, reason}]
    var crashEvents = [];          // [{ts, app, pid, exception, message}]
    var rawEvents = [];            // 所有原始事件（仅用于计数）
    var DEFAULT_LAST_DURATION = 1000;

    // ----- Toast -----
    function showToast(msg, duration) {
        duration = duration || 2500;
        var toast = document.getElementById('toast');
        toast.textContent = msg;
        toast.classList.add('show');
        clearTimeout(toast._timer);
        toast._timer = setTimeout(function() { toast.classList.remove('show'); }, duration);
    }

    // ----- 时间戳解析 -----
    function parseTimestamp(timeStr) {
        var pt = timeStr.match(/(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})\.(\d{3})/);
        if (!pt) return 0;
        var now = new Date();
        return new Date(
            now.getFullYear(),
            parseInt(pt[1]) - 1,
            parseInt(pt[2]),
            parseInt(pt[3]),
            parseInt(pt[4]),
            parseInt(pt[5]),
            parseInt(pt[6])
        ).getTime();
    }

    // ----- 解析日志（三种事件） -----
    function parseLog(text) {
        var lines = text.split(/\r?\n/);
        var resumeEvents = [];
        var anrs = [];
        var crashes = [];

        // 正则
        var resumeRegex = /(\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+\d+\s+\d+\s+I\s+wm_on_resume_called:\s+\[([^\]]*)\]/;
        var anrRegex = /(\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+\d+\s+\d+\s+\d+\s+I\s+am_anr\s*:\s*\[([^\]]*)\]/;
        var crashRegex = /(\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+\d+\s+\d+\s+\d+\s+I\s+am_crash\s*:\s*\[([^\]]*)\]/;

        for (var i = 0; i < lines.length; i++) {
            var line = lines[i];
            var m;

            // 1. wm_on_resume_called
            m = line.match(resumeRegex);
            if (m) {
                var ts = parseTimestamp(m[1]);
                var parts = m[2].split(',').map(function(s) { return s.trim(); });
                if (parts.length >= 2) {
                    resumeEvents.push({
                        ts: ts,
                        app: parts[1]           // Activity 全名
                    });
                }
                continue;
            }

            // 2. am_anr
            m = line.match(anrRegex);
            if (m) {
                var ts = parseTimestamp(m[1]);
                var parts = m[2].split(',').map(function(s) { return s.trim(); });
                if (parts.length >= 5) {
                    anrs.push({
                        ts: ts,
                        app: parts[2],           // package name
                        pid: parts[1],
                        reason: parts[4]
                    });
                }
                continue;
            }

            // 3. am_crash
            m = line.match(crashRegex);
            if (m) {
                var ts = parseTimestamp(m[1]);
                var parts = m[2].split(',').map(function(s) { return s.trim(); });
                if (parts.length >= 6) {
                    crashes.push({
                        ts: ts,
                        app: parts[2],           // process name (package)
                        pid: parts[1],
                        exception: parts[4],
                        message: parts[5]
                    });
                }
                continue;
            }
        }

        // 排序
        resumeEvents.sort(function(a, b) { return a.ts - b.ts; });
        anrs.sort(function(a, b) { return a.ts - b.ts; });
        crashes.sort(function(a, b) { return a.ts - b.ts; });

        return {
            resume: resumeEvents,
            anr: anrs,
            crash: crashes
        };
    }

    // ----- 构建区间（只基于 resume） -----
    function buildIntervals(resumeEvents) {
        if (!resumeEvents || resumeEvents.length === 0) return [];
        var intervals = [];
        for (var i = 0; i < resumeEvents.length; i++) {
            var startTs = resumeEvents[i].ts;
            var endTs;
            if (i < resumeEvents.length - 1) {
                endTs = resumeEvents[i+1].ts;
            } else {
                endTs = startTs + DEFAULT_LAST_DURATION;
            }
            intervals.push({
                app: resumeEvents[i].app,
                startTs: startTs,
                endTs: endTs
            });
        }
        return intervals;
    }

    // ----- 绘制时间线（含标记行） -----
    function drawTimeline(intervals, anrEvents, crashEvents) {
        if (!intervals || intervals.length === 0) {
            chartContainer.style.display = 'none';
            noDataMsg.style.display = 'block';
            statsArea.style.display = 'none';
            return;
        }
        chartContainer.style.display = 'block';
        noDataMsg.style.display = 'none';
        statsArea.style.display = 'flex';

        // 收集所有 Activity
        var appSet = {};
        for (var i = 0; i < intervals.length; i++) {
            appSet[intervals[i].app] = true;
        }
        var apps = Object.keys(appSet).sort();
        var appMap = {};
        for (var i = 0; i < apps.length; i++) {
            appMap[apps[i]] = i;
        }

        var minTs = intervals[0].startTs;
        var maxTs = intervals[intervals.length-1].endTs;
        var totalDuration = maxTs - minTs;

        // 更新统计
        eventCountEl.textContent = intervals.length + anrEvents.length + crashEvents.length;
        appCountEl.textContent = apps.length;
        timeSpanEl.textContent = totalDuration.toFixed(0) + ' ms';
        anrCountEl.textContent = anrEvents.length;
        crashCountEl.textContent = crashEvents.length;

        var colors = ['#3b82f6', '#22c55e', '#f59e0b', '#ef4444', '#8b5cf6', '#ec4899', '#14b8a6', '#f97316', '#6366f1', '#84cc16'];

        var html = '';

        // ---- 时间轴 ----
        html += '<div class="chart-time-axis">';
        var steps = [0, 25, 50, 75, 100];
        for (var s = 0; s < steps.length; s++) {
            var sec = (steps[s] / 100) * totalDuration / 1000;
            html += '<span>' + sec.toFixed(1) + 's</span>';
        }
        html += '</div>';

        // ---- 事件标记行（ANR / Crash） ----
        var allMarkers = anrEvents.concat(crashEvents);
        if (allMarkers.length > 0) {
            html += '<div class="chart-row-marker">';
            html += '<div class="chart-label-marker">⚠️ 事件标记</div>';
            html += '<div class="chart-track-marker">';
            for (var k = 0; k < allMarkers.length; k++) {
                var evt = allMarkers[k];
                var pct = ((evt.ts - minTs) / totalDuration) * 100;
                if (pct < 0) pct = 0;
                if (pct > 100) pct = 100;
                var cls = (evt.hasOwnProperty('reason')) ? 'chart-marker-anr' : 'chart-marker-crash';
                var title = (evt.hasOwnProperty('reason')) ?
                    'ANR: ' + evt.app + ' | ' + evt.reason :
                    'Crash: ' + evt.app + ' | ' + evt.exception + ' - ' + evt.message;
                html += '<div class="chart-marker ' + cls + '" style="left:' + pct + '%;" title="' + title + '"></div>';
            }
            html += '</div></div>';
        }

        // ---- Activity 行 ----
        for (var j = 0; j < apps.length; j++) {
            var app = apps[j];
            html += '<div class="chart-row">';
            var displayName = app.length > 36 ? app.substring(0, 34) + '…' : app;
            html += '<div class="chart-label" title="' + app + '">' + displayName + '</div>';
            html += '<div class="chart-track">';

            // 绘制该应用的所有区间
            for (var k = 0; k < intervals.length; k++) {
                var item = intervals[k];
                if (item.app !== app) continue;
                var leftPct = ((item.startTs - minTs) / totalDuration) * 100;
                var widthPct = ((item.endTs - item.startTs) / totalDuration) * 100;
                if (widthPct < 0.1) widthPct = 0.1;
                var colorIdx = appMap[app] % colors.length;
                html += '<div class="chart-bar" style="left:' + leftPct + '%; width:' + widthPct + '%; background:' + colors[colorIdx] + ';"></div>';
            }
            html += '</div></div>';
        }

        chartContainer.innerHTML = html;
    }

    // ----- 导出 Perfetto JSON（仅区间） -----
    function exportPerfetto(intervals) {
        if (!intervals || intervals.length === 0) {
            showToast('没有数据可导出', 1500);
            return;
        }

        try {
            var appSet = {};
            for (var i = 0; i < intervals.length; i++) {
                appSet[intervals[i].app] = true;
            }
            var apps = Object.keys(appSet);
            var pidMap = {};
            for (var i = 0; i < apps.length; i++) {
                pidMap[apps[i]] = 1000 + i;
            }

            var traceEvents = [];
            var tsOffset = intervals[0].startTs;
            for (var i = 0; i < intervals.length; i++) {
                var item = intervals[i];
                var pid = pidMap[item.app];
                traceEvents.push({
                    ph: 'X',
                    name: item.app,
                    pid: pid,
                    tid: pid,
                    ts: (item.startTs - tsOffset) * 1000,
                    dur: (item.endTs - item.startTs) * 1000,
                    cat: 'activity'
                });
            }
            for (var i = 0; i < apps.length; i++) {
                traceEvents.push({
                    ph: 'M',
                    pid: pidMap[apps[i]],
                    name: 'process_name',
                    args: { name: apps[i] }
                });
            }

            var jsonStr = JSON.stringify({ traceEvents: traceEvents }, null, 2);
            var fileName = 'android_timeline_' + new Date().toISOString().slice(0,10) + '.json';

            // ----- 方法1: Blob + URL -----
            try {
                var blob = new Blob([jsonStr], { type: 'application/json' });
                var url = URL.createObjectURL(blob);
                var a = document.createElement('a');
                a.href = url;
                a.download = fileName;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                setTimeout(function() { URL.revokeObjectURL(url); }, 5000);
                showToast('✅ JSON 已下载', 2000);
                return;
            } catch (e) {}

            // ----- 方法2: msSaveBlob (IE) -----
            if (window.navigator && window.navigator.msSaveBlob) {
                try {
                    var blob = new Blob([jsonStr], { type: 'application/json' });
                    window.navigator.msSaveBlob(blob, fileName);
                    showToast('✅ JSON 已下载', 2000);
                    return;
                } catch (e) {}
            }

            // ----- 方法3: data URI (window.open) -----
            try {
                var dataUri = 'data:application/json;charset=utf-8,' + encodeURIComponent(jsonStr);
                var win = window.open(dataUri, '_blank');
                if (!win) {
                    showToast('请复制 JSON 内容', 3000);
                    alert('请复制以下 JSON 内容并保存为 .json 文件：\n\n' + jsonStr);
                } else {
                    setTimeout(function() { win.close(); }, 5000);
                    showToast('✅ 已打开新窗口，请保存为 .json', 2000);
                }
                return;
            } catch (e) {}

            // ----- 方法4: 复制到剪贴板 -----
            showToast('无法自动下载，请复制 JSON 内容', 3000);
            var textarea = document.createElement('textarea');
            textarea.value = jsonStr;
            textarea.style.position = 'fixed';
            textarea.style.left = '-9999px';
            textarea.style.top = '0';
            document.body.appendChild(textarea);
            textarea.select();
            try {
                document.execCommand('copy');
                showToast('✅ JSON 已复制到剪贴板', 2000);
            } catch (e) {
                alert('请复制以下 JSON 内容并保存为 .json 文件：\n\n' + jsonStr);
            }
            document.body.removeChild(textarea);

        } catch (err) {
            showToast('导出失败: ' + err.message, 3000);
            console.error(err);
        }
    }

    // ----- 主流程 -----
    function processLog(text) {
        if (!text.trim()) { showToast('请输入或上传日志', 2000); return; }

        var parsed = parseLog(text);
        if (parsed.resume.length === 0) {
            showToast('未找到 wm_on_resume_called 事件', 3000);
            return;
        }

        intervals = buildIntervals(parsed.resume);
        anrEvents = parsed.anr;
        crashEvents = parsed.crash;
        drawTimeline(intervals, anrEvents, crashEvents);
        exportBtn.disabled = false;
        showToast('✅ 解析完成', 1500);
    }

    // ----- 事件绑定 -----
    parseBtn.addEventListener('click', function() { processLog(logInput.value); });

    fileInput.addEventListener('change', function(e) {
        var file = e.target.files[0];
        if (!file) return;
        var reader = new FileReader();
        reader.onload = function(ev) {
            logInput.value = ev.target.result;
            showToast('✅ 已加载：' + file.name, 1500);
            processLog(ev.target.result);
            fileInput.value = '';
        };
        reader.onerror = function() { showToast('读取失败', 2000); };
        reader.readAsText(file);
    });

    clearBtn.addEventListener('click', function() {
        logInput.value = '';
        intervals = [];
        anrEvents = [];
        crashEvents = [];
        chartContainer.innerHTML = '';
        chartContainer.style.display = 'none';
        noDataMsg.style.display = 'block';
        statsArea.style.display = 'none';
        exportBtn.disabled = true;
        showToast('已清空', 1000);
    });

    exportBtn.addEventListener('click', function() {
        if (intervals.length === 0) { showToast('没有数据', 1500); return; }
        exportPerfetto(intervals);
    });

    // 初始提示
    showToast('💡 上传或粘贴 events log，点击解析', 2500);
})();
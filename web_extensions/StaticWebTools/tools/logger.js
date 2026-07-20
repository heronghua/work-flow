// 日志工具 - 带时间戳，默认级别 info
window.logger = {
    _level: 4, // 默认 info: 0=silent, 1=error, 2=warn, 3=info, 4=debug
    _showTimestamp: true, // 是否显示时间戳

    // 设置日志级别（支持字符串或数字）
    setLevel(level) {
        const map = { silent: 0, error: 1, warn: 2, info: 3, debug: 4 };
        if (typeof level === 'string') {
            this._level = map[level.toLowerCase()] !== undefined ? map[level.toLowerCase()] : 3;
        } else {
            this._level = level;
        }
    },

    // 开启/关闭时间戳
    setTimestamp(show) {
        this._showTimestamp = show;
    },

    // 内部方法：格式化时间戳
    _getTimestamp() {
        return new Date().toISOString(); // 格式如 "2026-07-18T14:30:00.123Z"
    },

    // 内部方法：组装前缀
    _prefix(levelName) {
        const parts = [];
        if (this._showTimestamp) {
            parts.push(this._getTimestamp());
        }
        parts.push(`${levelName}`);
        return parts.join(' ');
    },

    error(...args) {
        if (this._level >= 1) {
            console.error(this._prefix('E'), ...args);
        }
    },
    warn(...args) {
        if (this._level >= 2) {
            console.warn(this._prefix('W'), ...args);
        }
    },
    info(...args) {
        if (this._level >= 3) {
            console.info(this._prefix('I'), ...args);
        }
    },
    debug(...args) {
        if (this._level >= 4) {
            console.debug(this._prefix('D'), ...args);
        }
    }
};
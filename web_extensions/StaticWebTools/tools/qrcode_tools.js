
        (function() {
			window.logger.warn("start");
			console.log("consolelog")
            "use strict";

            // ---------- DOM 引用 ----------
            const textInput = document.getElementById('textInput');
            const genBtn = document.getElementById('genQrCode');
            const canvas = document.getElementById('qrcodeCanvas');
            const generateResultDiv = document.getElementById('generateResult');

            const fileInput = document.getElementById('fileInput');
            const parseBtn = document.getElementById('parseBtn');
            const parseResultDiv = document.getElementById('parseResult');

            // ---------- 生成二维码 ----------
            function generateQR() {
                const text = textInput.value.trim();
                if (!text) {
                    alert('请先输入要生成二维码的内容！');
                    return;
                }

                // 清空 canvas 并重新绘制
                const ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                // 使用 qrcode 库生成到 canvas
                QRCode.toCanvas(canvas, text, {
                    width: 200,
                    margin: 2,
                    color: {
                        dark: '#1e293b',   // 深色模块
                        light: '#ffffff'   // 浅色背景
                    }
                }, function (error) {
                    if (error) {
                        console.error('生成失败:', error);
                        generateResultDiv.innerHTML = `<div class="error-msg">❌ 生成失败：${error.message}</div>`;
                    } else {
                        // 成功 – 在 canvas 下方显示一条成功消息（可选）
                        // 但 canvas 已绘制，我们保留 canvas 并清空可能存在的错误信息
                        const existingError = generateResultDiv.querySelector('.error-msg');
                        if (existingError) existingError.remove();
                        // 如果没有 note，可以补一个
                        let note = generateResultDiv.querySelector('.note');
                        if (!note) {
                            note = document.createElement('div');
                            note.className = 'note';
                            note.textContent = '✅ 二维码已生成';
                            generateResultDiv.appendChild(note);
                        } else {
                            note.textContent = '✅ 二维码已生成';
                        }
                    }
                });
            }

            // ---------- 解析二维码 ----------
            function parseQR() {
                const file = fileInput.files[0];
                if (!file) {
                    alert('请先选择一张包含二维码的图片！');
                    return;
                }

                // 检查是否为图片类型
                if (!file.type.startsWith('image/')) {
                    alert('请选择图片文件！');
                    return;
                }

                // 读取图片并绘制到临时 canvas 以获取像素数据
                const reader = new FileReader();
                reader.onload = function(e) {
                    const img = new Image();
                    img.onload = function() {
                        // 创建临时 canvas 来提取图像数据
                        const tempCanvas = document.createElement('canvas');
                        const tempCtx = tempCanvas.getContext('2d');
                        // 为了解析准确，建议尺寸不要太小，但也要控制性能
                        let width = img.width;
                        let height = img.height;
                        // 如果图片太大，缩小到 800px 以内（jsQR 对较大图像也能处理，但可能慢）
                        const MAX_SIZE = 800;
                        if (width > MAX_SIZE || height > MAX_SIZE) {
                            const scale = Math.min(MAX_SIZE / width, MAX_SIZE / height);
                            width = Math.round(width * scale);
                            height = Math.round(height * scale);
                        }
                        tempCanvas.width = width;
                        tempCanvas.height = height;
                        tempCtx.drawImage(img, 0, 0, width, height);

                        // 获取 ImageData
                        const imageData = tempCtx.getImageData(0, 0, width, height);
                        // 使用 jsQR 解析
                        const code = jsQR(imageData.data, imageData.width, imageData.height, {
                            inversionAttempts: "dontInvert", // 尝试自动检测反转，但这里保持原样
                        });

                        // 显示结果
                        if (code && code.data) {
                            parseResultDiv.innerHTML = `
                                <div class="parse-result">
                                    <strong>✅ 解析成功</strong><br>
                                    <span style="color: #0f172a;">${escapeHtml(code.data)}</span>
                                </div>
                            `;
                        } else {
                            parseResultDiv.innerHTML = `<div class="error-msg">❌ 未能识别二维码，请尝试其他图片或调整光线。</div>`;
                        }
                    };
                    img.onerror = function() {
                        parseResultDiv.innerHTML = `<div class="error-msg">❌ 图片加载失败，请确认文件是否损坏。</div>`;
                    };
                    img.src = e.target.result;
                };
                reader.onerror = function() {
                    parseResultDiv.innerHTML = `<div class="error-msg">❌ 读取文件失败，请重试。</div>`;
                };
                reader.readAsDataURL(file);
            }

            // 简单的防 XSS 转义（用于显示用户内容）
            function escapeHtml(unsafe) {
                return unsafe
                    .replace(/&/g, "&amp;")
                    .replace(/</g, "&lt;")
                    .replace(/>/g, "&gt;")
                    .replace(/"/g, "&quot;")
                    .replace(/'/g, "&#039;");
            }

            // ---------- 绑定事件 ----------
            genBtn.addEventListener('click', generateQR);
            parseBtn.addEventListener('click', parseQR);

            // 支持回车键生成
            textInput.addEventListener('keypress', function(e) {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    generateQR();
                }
            });

            // 文件选择后自动解析（可选），但保留手动按钮
            // 如果你希望选择后自动解析，可以取消下面注释：
            // fileInput.addEventListener('change', parseQR);

            // ---------- 页面加载后默认生成一个示例 ----------
            window.addEventListener('load', function() {
                // 延迟一点点让库加载完成
                setTimeout(generateQR, 100);
            });

        })();
    

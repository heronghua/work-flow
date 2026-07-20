// ============================================================
// PNG查看器 (ES5, 无箭头函数, 无let/const)
// 内存优化版：主动释放ObjectURL，取色器限制canvas尺寸
// ============================================================

(function() {
    "use strict";

    var PNGViewer = {

        // 初始化
        init: function() {
            window.logger.debug("PNGViewer.init +");
            this.cacheElements();
            this.bindEvents();
            this.setBackground('checkerboard');
            window.logger.debug("PNGViewer.init -");
        },

        // 缓存DOM元素
        cacheElements: function() {
            this.folderInput = document.getElementById('folder-input');
            this.folderBtn = document.getElementById('folder-btn');
            this.bgOptions = document.querySelectorAll('.bg-option');
            this.imageGridContainer = document.getElementById('image-grid-container');
            this.imageGrid = document.getElementById('image-grid');
            this.noImages = document.getElementById('no-images');
            this.fileCount = document.getElementById('file-count');
            this.currentFile = document.getElementById('current-file');
            this.fileSize = document.getElementById('file-size');
            this.imageDimensions = document.getElementById('image-dimensions');
            this.selectedIndex = document.getElementById('selected-index');
            this.imageCount = document.getElementById('image-count');
            this.lockedSize = document.getElementById('locked-size');
            this.filteredCount = document.getElementById('filtered-count');
            this.lockSizeBtn = document.getElementById('lock-size-btn');
            this.fpsInput = document.getElementById('fps-input');
            this.zoomInfo = document.getElementById('zoom-info');

            this.modal = document.getElementById('image-modal');
            this.modalBg = document.getElementById('modal-bg');
            this.modalImage = document.getElementById('modal-image');
            this.modalClose = document.getElementById('modal-close');
            this.modalInfo = document.getElementById('modal-info');
            this.modalPrev = document.getElementById('modal-prev');
            this.modalPlay = document.getElementById('modal-play');
            this.modalPause = document.getElementById('modal-pause');
            this.modalNext = document.getElementById('modal-next');
            this.modalStop = document.getElementById('modal-stop');
            this.modalLockBtn = document.getElementById('modal-lock-btn');

            this.colorPicker = document.getElementById('color-picker');
            this.colorPreview = document.getElementById('color-preview');
            this.colorInfo = document.getElementById('color-info');

            this.canvas = document.createElement('canvas');
            this.ctx = this.canvas.getContext('2d');
        },

        // 绑定事件
        bindEvents: function() {
            logger.debug("bindEvents +");
            var self = this;

            this.folderBtn.addEventListener('click', function() {
                self.folderInput.click();
            });
            this.folderInput.addEventListener('change', function(e) {
                self.handleFolderSelect(e);
            });

            // 背景选项
            for (var i = 0; i < this.bgOptions.length; i++) {
                (function(option) {
                    option.addEventListener('click', function() {
                        var bgType = option.getAttribute('data-bg');
                        self.setBackground(bgType);
                        for (var j = 0; j < self.bgOptions.length; j++) {
                            self.bgOptions[j].classList.remove('active');
                        }
                        option.classList.add('active');
                    });
                })(this.bgOptions[i]);
            }

            this.lockSizeBtn.addEventListener('click', function() {
                self.toggleSizeLock();
            });

            // 模态框事件
            this.modalClose.addEventListener('click', function() {
                self.closeModal();
            });
            this.modalLockBtn.addEventListener('click', function() {
                self.lockSizeFromModal();
            });
            this.modalPrev.addEventListener('click', function() {
                self.navigateModal('prev');
            });
            this.modalNext.addEventListener('click', function() {
                self.navigateModal('next');
            });
            this.modalPlay.addEventListener('click', function() {
                self.startModalPlayback();
            });
            this.modalPause.addEventListener('click', function() {
                self.stopModalPlayback();
            });
            this.modalStop.addEventListener('click', function() {
                self.stopModal();
            });

            // 取色器事件
            this.modalImage.addEventListener('mousemove', function(e) {
                self.handleColorPicker(e);
            });
            this.modalImage.addEventListener('mouseleave', function() {
                self.hideColorPicker();
            });

            // 缩放控制 (Ctrl+滚轮)
            document.addEventListener('keydown', function(e) {
                if (e.key === 'Control') self.isCtrlPressed = true;
            });
            document.addEventListener('keyup', function(e) {
                if (e.key === 'Control') self.isCtrlPressed = false;
            });
            this.imageGridContainer.addEventListener('wheel', function(e) {
                self.handleZoom(e);
            });
            logger.debug("bindEvents -");
        },

        // ---------- 内存释放辅助 ----------
        revokeGridURLs: function() {
            // 释放网格中所有图片的 ObjectURL
            var images = this.imageGrid.querySelectorAll('.grid-image');
            for (var i = 0; i < images.length; i++) {
                var src = images[i].src;
                if (src && src.indexOf('blob:') === 0) {
                    URL.revokeObjectURL(src);
                }
            }
            // 释放模态框中的旧 URL
            if (this.modalImage.src && this.modalImage.src.indexOf('blob:') === 0) {
                URL.revokeObjectURL(this.modalImage.src);
                this.modalImage.src = '';
            }
        },

        // 状态
        state: {
            pngFiles: [],
            currentBackground: 'checkerboard',
            selectedItem: null,
            lockedSizeValue: null,
            filteredFiles: [],
            currentModalIndex: 0,
            modalPlaybackInterval: null,
            isModalPlaying: false,
            currentModalSize: { width: 0, height: 0 },
            scale: 1,
            isCtrlPressed: false
        },

        // 处理文件夹选择
        handleFolderSelect: function(event) {
            window.logger.info("handleFolderSelect +");
            var files = event.target.files;
            var pngFiles = [];
            for (var i = 0; i < files.length; i++) {
                if (files[i].type === 'image/png') {
                    pngFiles.push(files[i]);
                }
            }

            pngFiles.sort(function(a, b) {
                return a.name.localeCompare(b.name);
            });

            // 释放旧网格的 URL
            this.revokeGridURLs();

            this.state.pngFiles = pngFiles;
            this.fileCount.textContent = '已加载PNG文件: ' + this.state.pngFiles.length;
            this.imageCount.textContent = '共 ' + this.state.pngFiles.length + ' 个图像';

            this.resetState();

            if (this.state.pngFiles.length === 0) {
                this.showNoImagesMessage();
                return;
            }

            this.renderImageGrid();
            window.logger.info("handleFolderSelect -");
        },

        resetState: function() {
            this.state.lockedSizeValue = null;
            this.lockedSize.textContent = '锁定尺寸: 无';
            this.lockSizeBtn.disabled = (this.state.pngFiles.length === 0);
            this.lockSizeBtn.classList.remove('locked');
            this.lockSizeBtn.textContent = '锁定当前尺寸';
        },

        showNoImagesMessage: function() {
            this.noImages.style.display = 'block';
            this.imageGrid.innerHTML = '';
            var noImgDiv = document.createElement('div');
            noImgDiv.className = 'no-images';
            noImgDiv.textContent = '请选择一个包含PNG图像的文件夹';
            this.imageGrid.appendChild(noImgDiv);

            this.currentFile.textContent = '当前文件: 无';
            this.fileSize.textContent = '文件大小: 无';
            this.imageDimensions.textContent = '图像尺寸: 无';
            this.selectedIndex.textContent = '选中索引: 无';
            this.filteredCount.textContent = '过滤后数量: 0';
        },

        renderImageGrid: function() {
            var self = this;
            this.noImages.style.display = 'none';
            this.imageGrid.innerHTML = ''; // 会移除子元素，但URL已在上层释放

            // 加载指示器
            var loadingDiv = document.createElement('div');
            loadingDiv.style.gridColumn = '1 / -1';
            loadingDiv.style.textAlign = 'center';
            loadingDiv.style.padding = '20px';
            loadingDiv.innerHTML = '<div class="loading"></div><p style="margin-top:10px;">加载图片中...</p>';
            this.imageGrid.appendChild(loadingDiv);

            var loadedCount = 0;
            var totalFiles = this.state.pngFiles.length;

            for (var i = 0; i < totalFiles; i++) {
                (function(index) {
                    var file = self.state.pngFiles[index];
                    logger.debug(file.name);
                    var img = new Image();
                    img.src = URL.createObjectURL(file);

                    img.onload = function() {
                        loadedCount++;
                        self.createGridItem(file, index, this.naturalWidth, this.naturalHeight);
                        if (loadedCount === totalFiles) {
                            self.finishImageLoading();
                        }
                    };
                    img.onerror = function() {
                        loadedCount++;
                        if (loadedCount === totalFiles) {
                            self.finishImageLoading();
                        }
                    };
                })(i);
            }
        },

        createGridItem: function(file, index, width, height) {
            var maxDisplaySize = 200;
            var displayWidth = width;
            var displayHeight = height;

            if (width > height) {
                if (width > maxDisplaySize) {
                    displayWidth = maxDisplaySize;
                    displayHeight = Math.round((height / width) * maxDisplaySize);
                }
            } else {
                if (height > maxDisplaySize) {
                    displayHeight = maxDisplaySize;
                    displayWidth = Math.round((width / height) * maxDisplaySize);
                }
            }

            var gridItem = document.createElement('div');
            gridItem.className = 'grid-item';
            gridItem.setAttribute('data-index', index);
            gridItem.setAttribute('data-width', width);
            gridItem.setAttribute('data-height', height);
            gridItem.setAttribute('data-filename', file.name);

            var itemContainer = document.createElement('div');
            itemContainer.className = 'item-container';
            itemContainer.style.width = displayWidth + 'px';
            itemContainer.style.height = displayHeight + 'px';

            var itemBg = document.createElement('div');
            itemBg.className = 'item-bg ' + this.state.currentBackground;
            itemBg.style.width = '100%';
            itemBg.style.height = '100%';

            var gridImage = document.createElement('img');
            gridImage.className = 'grid-image';
            gridImage.src = URL.createObjectURL(file);
            gridImage.alt = file.name;
            gridImage.style.width = displayWidth + 'px';
            gridImage.style.height = displayHeight + 'px';

            var fileName = document.createElement('div');
            fileName.className = 'item-filename';
            fileName.textContent = file.name;
            fileName.style.width = displayWidth + 'px';

            itemContainer.appendChild(itemBg);
            itemContainer.appendChild(gridImage);
            gridItem.appendChild(itemContainer);
            gridItem.appendChild(fileName);

            var self = this;
            gridItem.addEventListener('click', function() {
                self.selectGridItem(gridItem, file, index, width, height);
            });
            gridItem.addEventListener('dblclick', function() {
                self.openModal(index);
            });

            this.imageGrid.appendChild(gridItem);
        },

        finishImageLoading: function() {
            var loadingElement = this.imageGrid.querySelector('div[style*="grid-column: 1 / -1"]');
            if (loadingElement) loadingElement.parentNode.removeChild(loadingElement);

            // 排序网格项，保证显示顺序与文件名一致
            var items = this.imageGrid.querySelectorAll('.grid-item');
            var itemsArray = Array.prototype.slice.call(items);
            itemsArray.sort(function(a, b) {
                return parseInt(a.getAttribute('data-index')) - parseInt(b.getAttribute('data-index'));
            });
            for (var i = 0; i < itemsArray.length; i++) {
                this.imageGrid.appendChild(itemsArray[i]);
            }

            if (this.state.lockedSizeValue) {
                this.applySizeFilter();
            }

            var firstItem = this.imageGrid.querySelector('.grid-item');
            if (firstItem) {
                var self = this;
                setTimeout(function() {
                    var index = parseInt(firstItem.getAttribute('data-index'));
                    var file = self.state.pngFiles[index];
                    var width = parseInt(firstItem.getAttribute('data-width'));
                    var height = parseInt(firstItem.getAttribute('data-height'));
                    self.selectGridItem(firstItem, file, index, width, height);
                }, 100);
            }
        },

        selectGridItem: function(gridItem, file, index, width, height) {
            if (this.state.selectedItem) {
                this.state.selectedItem.classList.remove('active');
            }
            gridItem.classList.add('active');
            this.state.selectedItem = gridItem;

            this.currentFile.textContent = '当前文件: ' + file.name;
            this.fileSize.textContent = '文件大小: ' + this.formatFileSize(file.size);
            this.imageDimensions.textContent = '图像尺寸: ' + width + ' × ' + height + ' 像素';

            if (this.state.lockedSizeValue) {
                this.selectedIndex.textContent = '选中索引: ' + (index + 1) + ' / ' + this.state.filteredFiles.length;
            } else {
                this.selectedIndex.textContent = '选中索引: ' + (index + 1) + ' / ' + this.state.pngFiles.length;
            }

            this.lockSizeBtn.disabled = false;
        },

        setBackground: function(type) {
            this.state.currentBackground = type;
            var bgElements = document.querySelectorAll('.item-bg');
            for (var i = 0; i < bgElements.length; i++) {
                bgElements[i].className = 'item-bg';
                bgElements[i].classList.add(type);
            }
            if (this.modal.style.display === 'flex') {
                this.modalBg.className = 'modal-bg';
                this.modalBg.classList.add(type);
            }
        },

        toggleSizeLock: function() {
            if (!this.state.selectedItem) return;

            if (this.lockSizeBtn.classList.contains('locked')) {
                this.state.lockedSizeValue = null;
                this.lockedSize.textContent = '锁定尺寸: 无';
                var allItems = this.imageGrid.querySelectorAll('.grid-item');
                for (var i = 0; i < allItems.length; i++) {
                    allItems[i].style.display = 'flex';
                }
                this.filteredCount.textContent = '过滤后数量: ' + allItems.length;
                this.lockSizeBtn.classList.remove('locked');
                this.lockSizeBtn.textContent = '锁定当前尺寸';
                this.modalLockBtn.textContent = '锁定此尺寸';
                this.modalLockBtn.disabled = false;
            } else {
                var width = this.state.selectedItem.getAttribute('data-width');
                var height = this.state.selectedItem.getAttribute('data-height');
                this.state.lockedSizeValue = width + 'x' + height;
                this.lockedSize.textContent = '锁定尺寸: ' + width + ' × ' + height;
                this.applySizeFilter();
                this.lockSizeBtn.classList.add('locked');
                this.lockSizeBtn.textContent = '解除锁定';
            }
        },

        lockSizeFromModal: function() {
            if (this.state.currentModalSize.width && this.state.currentModalSize.height) {
                var w = this.state.currentModalSize.width;
                var h = this.state.currentModalSize.height;
                this.state.lockedSizeValue = w + 'x' + h;
                this.lockedSize.textContent = '锁定尺寸: ' + w + ' × ' + h;
                this.applySizeFilter();
                this.lockSizeBtn.classList.add('locked');
                this.lockSizeBtn.textContent = '解除锁定';
                this.modalLockBtn.textContent = '已锁定此尺寸';
                this.modalLockBtn.disabled = true;
            }
        },

        applySizeFilter: function() {
            if (!this.state.lockedSizeValue) return;
            var parts = this.state.lockedSizeValue.split('x');
            var targetWidth = parseInt(parts[0]);
            var targetHeight = parseInt(parts[1]);
            var allItems = this.imageGrid.querySelectorAll('.grid-item');
            var visibleCount = 0;
            var filteredFiles = [];

            for (var i = 0; i < allItems.length; i++) {
                var item = allItems[i];
                var w = parseInt(item.getAttribute('data-width'));
                var h = parseInt(item.getAttribute('data-height'));
                if (w === targetWidth && h === targetHeight) {
                    item.style.display = 'flex';
                    visibleCount++;
                    var filename = item.getAttribute('data-filename');
                    var file = this.state.pngFiles.filter(function(f) { return f.name === filename; })[0];
                    if (file) filteredFiles.push(file);
                } else {
                    item.style.display = 'none';
                }
            }
            this.filteredCount.textContent = '过滤后数量: ' + visibleCount;
            this.state.filteredFiles = filteredFiles;
            this.state.filteredFiles.sort(function(a, b) { return a.name.localeCompare(b.name); });

            if (!this.state.selectedItem || this.state.selectedItem.style.display === 'none') {
                var firstVisible = this.imageGrid.querySelector('.grid-item[style*="display: flex"]');
                if (firstVisible) firstVisible.click();
            }
        },

        // ---------- 模态框操作（含内存释放） ----------
        openModal: function(index) {
            // 释放旧的模态 URL
            if (this.modalImage.src && this.modalImage.src.indexOf('blob:') === 0) {
                URL.revokeObjectURL(this.modalImage.src);
            }

            this.state.currentModalIndex = index;
            var file = this.state.pngFiles[index];
            var item = document.querySelector('.grid-item[data-index="' + index + '"]');
            if (!item) return; // 防止意外
            var width = parseInt(item.getAttribute('data-width'));
            var height = parseInt(item.getAttribute('data-height'));
            this.state.currentModalSize = { width: width, height: height };

            this.modalImage.src = URL.createObjectURL(file);
            this.modalInfo.textContent = file.name + ' (' + (index + 1) + '/' + this.state.pngFiles.length + ')';

            this.modalBg.className = 'modal-bg';
            this.modalBg.classList.add(this.state.currentBackground);

            if (this.state.lockedSizeValue === (width + 'x' + height)) {
                this.modalLockBtn.textContent = '已锁定此尺寸';
                this.modalLockBtn.disabled = true;
            } else {
                this.modalLockBtn.textContent = '锁定此尺寸';
                this.modalLockBtn.disabled = false;
            }

            this.modal.style.display = 'flex';
            this.stopModalPlayback();
        },

        closeModal: function() {
            this.modal.style.display = 'none';
            this.stopModalPlayback();
            this.hideColorPicker();
            // 模态框关闭时不释放 URL，因为可能再次打开同一张图；但也可选择释放，下次重新创建。
            // 这里不释放，让 openModal 去释放旧的。
        },

        stopModal: function() {
            this.modal.style.display = 'none';
            this.stopModalPlayback();
            this.hideColorPicker();
        },

        navigateModal: function(direction) {
            if (this.state.pngFiles.length === 0) return;
            var nextIndex;
            var self = this;

            if (this.state.lockedSizeValue) {
                var visibleItems = [];
                var allItems = this.imageGrid.querySelectorAll('.grid-item');
                for (var i = 0; i < allItems.length; i++) {
                    if (allItems[i].style.display !== 'none') {
                        visibleItems.push(allItems[i]);
                    }
                }
                if (visibleItems.length === 0) return;
                var currentVisibleIndex = -1;
                for (var j = 0; j < visibleItems.length; j++) {
                    if (parseInt(visibleItems[j].getAttribute('data-index')) === this.state.currentModalIndex) {
                        currentVisibleIndex = j;
                        break;
                    }
                }
                if (currentVisibleIndex === -1) return;
                if (direction === 'prev') {
                    nextIndex = (currentVisibleIndex - 1 + visibleItems.length) % visibleItems.length;
                } else {
                    nextIndex = (currentVisibleIndex + 1) % visibleItems.length;
                }
                this.state.currentModalIndex = parseInt(visibleItems[nextIndex].getAttribute('data-index'));
            } else {
                if (direction === 'prev') {
                    this.state.currentModalIndex = (this.state.currentModalIndex - 1 + this.state.pngFiles.length) % this.state.pngFiles.length;
                } else {
                    this.state.currentModalIndex = (this.state.currentModalIndex + 1) % this.state.pngFiles.length;
                }
            }

            // 释放旧的模态 URL
            if (this.modalImage.src && this.modalImage.src.indexOf('blob:') === 0) {
                URL.revokeObjectURL(this.modalImage.src);
            }

            var file = this.state.pngFiles[this.state.currentModalIndex];
            var item = document.querySelector('.grid-item[data-index="' + this.state.currentModalIndex + '"]');
            if (!item) return;
            var width = parseInt(item.getAttribute('data-width'));
            var height = parseInt(item.getAttribute('data-height'));
            this.state.currentModalSize = { width: width, height: height };
            this.modalImage.src = URL.createObjectURL(file);
            this.modalInfo.textContent = file.name + ' (' + (this.state.currentModalIndex + 1) + '/' + this.state.pngFiles.length + ')';
        },

        startModalPlayback: function() {
            if (this.state.pngFiles.length === 0) return;
            this.state.isModalPlaying = true;
            this.modalPlay.disabled = true;
            this.modalPause.disabled = false;

            if (this.state.modalPlaybackInterval) {
                clearInterval(this.state.modalPlaybackInterval);
            }

            var fps = parseInt(this.fpsInput.value) || 33;
            var interval = 1000 / fps;
            var self = this;

            this.state.modalPlaybackInterval = setInterval(function() {
                var nextIndex;
                if (self.state.lockedSizeValue) {
                    var visibleItems = [];
                    var allItems = self.imageGrid.querySelectorAll('.grid-item');
                    for (var i = 0; i < allItems.length; i++) {
                        if (allItems[i].style.display !== 'none') {
                            visibleItems.push(allItems[i]);
                        }
                    }
                    if (visibleItems.length === 0) {
                        self.stopModalPlayback();
                        return;
                    }
                    var currentVisibleIndex = -1;
                    for (var j = 0; j < visibleItems.length; j++) {
                        if (parseInt(visibleItems[j].getAttribute('data-index')) === self.state.currentModalIndex) {
                            currentVisibleIndex = j;
                            break;
                        }
                    }
                    if (currentVisibleIndex === -1) return;
                    nextIndex = (currentVisibleIndex + 1) % visibleItems.length;
                    self.state.currentModalIndex = parseInt(visibleItems[nextIndex].getAttribute('data-index'));
                } else {
                    self.state.currentModalIndex = (self.state.currentModalIndex + 1) % self.state.pngFiles.length;
                }

                // 释放旧的模态 URL 并更新
                if (self.modalImage.src && self.modalImage.src.indexOf('blob:') === 0) {
                    URL.revokeObjectURL(self.modalImage.src);
                }
                var file = self.state.pngFiles[self.state.currentModalIndex];
                var item = document.querySelector('.grid-item[data-index="' + self.state.currentModalIndex + '"]');
                if (!item) {
                    self.stopModalPlayback();
                    return;
                }
                var width = parseInt(item.getAttribute('data-width'));
                var height = parseInt(item.getAttribute('data-height'));
                self.state.currentModalSize = { width: width, height: height };
                self.modalImage.src = URL.createObjectURL(file);
                self.modalInfo.textContent = file.name + ' (' + (self.state.currentModalIndex + 1) + '/' + self.state.pngFiles.length + ')';
            }, interval);
        },

        stopModalPlayback: function() {
            this.state.isModalPlaying = false;
            this.modalPlay.disabled = false;
            this.modalPause.disabled = true;
            if (this.state.modalPlaybackInterval) {
                clearInterval(this.state.modalPlaybackInterval);
                this.state.modalPlaybackInterval = null;
            }
        },

        // ---------- 取色器（优化canvas尺寸） ----------
        handleColorPicker: function(e) {
            if (!this.modalImage.complete || this.modalImage.naturalWidth === 0) return;
            this.colorPicker.style.display = 'block';

            var rect = this.modalImage.getBoundingClientRect();
            var x = e.clientX - rect.left;
            var y = e.clientY - rect.top;

            // 限制采样尺寸，提升性能
            var maxSampleSize = 1000;
            var srcWidth = this.modalImage.naturalWidth;
            var srcHeight = this.modalImage.naturalHeight;
            var sampleWidth, sampleHeight;
            if (srcWidth > srcHeight) {
                sampleWidth = Math.min(srcWidth, maxSampleSize);
                sampleHeight = Math.round(srcHeight * (sampleWidth / srcWidth));
            } else {
                sampleHeight = Math.min(srcHeight, maxSampleSize);
                sampleWidth = Math.round(srcWidth * (sampleHeight / srcHeight));
            }

            this.canvas.width = sampleWidth;
            this.canvas.height = sampleHeight;
            this.ctx.drawImage(this.modalImage, 0, 0, sampleWidth, sampleHeight);

            // 将鼠标坐标映射到原图坐标
            var scaleX = srcWidth / rect.width;
            var scaleY = srcHeight / rect.height;
            var pixelX = Math.floor(x * scaleX);
            var pixelY = Math.floor(y * scaleY);

            // 从采样 canvas 中取色（需要映射到采样尺寸）
            var sampleScaleX = sampleWidth / srcWidth;
            var sampleScaleY = sampleHeight / srcHeight;
            var samplePixelX = Math.floor(pixelX * sampleScaleX);
            var samplePixelY = Math.floor(pixelY * sampleScaleY);

            // 边界保护
            if (samplePixelX >= 0 && samplePixelX < sampleWidth && samplePixelY >= 0 && samplePixelY < sampleHeight) {
                var pixelData = this.ctx.getImageData(samplePixelX, samplePixelY, 1, 1).data;
                var r = pixelData[0];
                var g = pixelData[1];
                var b = pixelData[2];
                var a = pixelData[3];
                this.colorPreview.style.backgroundColor = 'rgba(' + r + ',' + g + ',' + b + ',' + a + ')';
                this.colorInfo.textContent = 'RGBA: ' + r + ', ' + g + ', ' + b + ', ' + a;
            } else {
                // 如果映射超出范围，隐藏取色器
                this.colorPicker.style.display = 'none';
                return;
            }

            this.colorPicker.style.top = (e.clientY - rect.top + 10) + 'px';
            this.colorPicker.style.left = (e.clientX - rect.left + 10) + 'px';
        },

        hideColorPicker: function() {
            this.colorPicker.style.display = 'none';
        },

        // ---------- 缩放（无改动） ----------
        handleZoom: function(e) {
            if (!this.state.isCtrlPressed) return;
            e.preventDefault();

            var delta = e.deltaY > 0 ? -0.1 : 0.1;
            this.state.scale = Math.min(Math.max(0.5, this.state.scale + delta), 3);
            this.imageGrid.style.transform = 'scale(' + this.state.scale + ')';
            this.zoomInfo.textContent = '缩放: ' + Math.round(this.state.scale * 100) + '%';
            this.zoomInfo.style.display = 'block';
            clearTimeout(this.zoomInfo.timeout);
            this.zoomInfo.timeout = setTimeout(function() {
                this.zoomInfo.style.display = 'none';
            }.bind(this), 3000);
        },

        formatFileSize: function(bytes) {
            if (bytes === 0) return '0 Bytes';
            var k = 1024;
            var sizes = ['Bytes', 'KB', 'MB', 'GB'];
            var i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }
    };

    // 初始化
    document.addEventListener('DOMContentLoaded', function() {
        window.logger.info("[DOMContentLoaded] +");
        PNGViewer.init();
        window.logger.info("[DOMContentLoaded] -");
    });
})();
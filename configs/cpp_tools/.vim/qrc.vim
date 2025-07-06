augroup compileAndRun
    noremap <leader>r :!make NV212PNG 1>/dev/null&&clear && /sbin/NV212PNG.exe test_files<CR>
augroup END


augroup compileAndRun
    noremap <leader>r :!make NV212PNG 1>/dev/null&&clear && /usr/bin/NV212PNG test_files<CR>
augroup END


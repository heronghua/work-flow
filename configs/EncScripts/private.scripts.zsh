M_DIR="${0:A:h}"
ENC_DIR="${M_DIR}"
DEC_DIR="${M_DIR}/../DecScripts"
set_log_level debug
decrypt_enc_scripts() {
    log_debug "M_DIR is $M_DIR , ENC_DIR is $ENC_DIR DEC_DIR is $DEC_DIR"
    mkdir -p "${DEC_DIR}"

    for enc_file in "${ENC_DIR}"/*.enc; do
        [[ -f "${enc_file}" ]] || continue

        local filename=$(basename "${enc_file}")
        local out_file="${DEC_DIR}/${filename%.enc}"

        if [[ -f "${out_file}" ]]; then
            echo "skip: ${out_file} already exists"
            continue
        fi

        echo "decrypting: ${enc_file} -> ${out_file}"
        openssl enc -d -aes-256-cbc -in "${enc_file}" -out "${out_file}"
    done
}

encrypt_dec_scripts() {
    mkdir -p "${ENC_DIR}"

    for src in "${DEC_DIR}"/*(N.); do
        [[ -f "${src}" ]] || continue
        local name=$(basename "${src}")
        local out="${ENC_DIR}/${name}.enc"
        echo "encrypt: ${src} -> ${out}"
        openssl enc -aes-256-cbc -salt -in "${src}" -out "${out}"
    done
}


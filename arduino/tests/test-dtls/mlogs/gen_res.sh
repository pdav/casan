#!/bin/bash

LOGDIR="logs/"
TMPDIR="tmplogs/"
RESDIR="reslogs/"
RSCRIPTS_DIR="r_scripts/"

PKT_SIZE="
85
1
"

function mk_enc_dec_files {

for i in "$@"
do

    grep Encryption "${LOGDIR}/${i}" | sed "s/Encryption took : //" \
        > "${TMPDIR}/${i}.enc"
    grep Decryption "${LOGDIR}/${i}" | sed "s/Decryption took : //" \
        > "${TMPDIR}/${i}.dec"

done
}

function mk_duration_files {

for i in "$@"
do

    grep duration "${LOGDIR}/${i}" | sed "s/duration: //" \
        > "${TMPDIR}/${i}.dur"

done

}

function mk_graphe_decryption {
    Rscript ${RSCRIPTS_DIR}/r_script_dec.r "${1}" "${2}"

    if [ -e "Rplots.pdf" ]
    then
        mv Rplots.pdf "${RESDIR}/res_${size}_dec.pdf"
    fi
}

function mk_graphe_encryption {

    Rscript ${RSCRIPTS_DIR}/r_script_enc.r "${1}" "${2}"

    if [ -e "Rplots.pdf" ]
    then
        mv Rplots.pdf "${RESDIR}/res_${size}_enc.pdf"
    fi
}

function mk_graphe_duration {
    Rscript ${RSCRIPTS_DIR}/r_script_duree.r "${1}" "${2}" "${3}"

    if [ -e "Rplots.pdf" ]
    then
        mv Rplots.pdf "${RESDIR}/res_${size}_duree.pdf"
    fi
}

function mk_sum_dec {
    Rscript ${RSCRIPTS_DIR}/r_script_dec_sum.r "${1}" "${2}" > "${3}"
}

function mk_sum_enc {
    Rscript ${RSCRIPTS_DIR}/r_script_enc_sum.r "${1}" "${2}" > "${3}"
}

function mk_sum_dur {
    Rscript ${RSCRIPTS_DIR}/r_script_duree_sum.r "${1}" "${2}" "${3}" > "${4}"
}

function process_rscripts_enc_dec {
# to process all the r_scripts
for size in $PKT_SIZE
do
    #echo "$i" $NB
    #NB=$(echo "$NB + 1" | bc)

    F1="log_${size}_dtls"
    F2="log_${size}_simplified"

    MISSING="no"

    for x in $F1 $F2
    do
        if [ ! -e "$LOGDIR/${x}" ]
        then
            echo "${x} missing"
            MISSING="yes"
        fi
    done

    if [ ${MISSING} = "yes" ]
    then
        continue
    fi

    mk_enc_dec_files "${F1}" "${F2}"

    TMPF1="${TMPDIR}/log_${size}_dtls"
    TMPF2="${TMPDIR}/log_${size}_simplified"

    mk_graphe_decryption  "${TMPF1}.dec" "${TMPF2}.dec"
    mk_graphe_encryption  "${TMPF1}.enc" "${TMPF2}.enc"

    mk_sum_dec "${TMPF1}.dec" "${TMPF2}.dec" "${RESDIR}/res_${size}_dec_sum"
    mk_sum_enc "${TMPF1}.enc" "${TMPF2}.enc" "${RESDIR}/res_${size}_enc_sum"

done
}

function process_rscripts_duration {
# to process the duration r_scripts
for size in $PKT_SIZE
do

    F1="log_${size}_dtls"
    F2="log_${size}_simplified"
    F3="log_${size}_none"

    mk_duration_files "${F1}" "${F2}" "${F3}"

    TMPF1="${TMPDIR}/log_${size}_dtls"
    TMPF2="${TMPDIR}/log_${size}_simplified"
    TMPF3="${TMPDIR}/log_${size}_none"

    mk_graphe_duration "${TMPF1}.dur" "${TMPF2}.dur" "${TMPF3}.dur"
    mk_sum_dur "${TMPF1}.dur" "${TMPF2}.dur" "${TMPF3}.dur" \
        "${RESDIR}/res_${size}_duree_sum"
done
}

function mk_dirs {
# to create the directories if not present
for i in "$@"
do
    if [ ! -d "${i}" ]
    then
        echo "${i} missing"
        mkdir "${i}"
    fi
done
}

# SCRIPT start

mk_dirs $LOGDIR $TMPDIR $RESDIR

process_rscripts_enc_dec
process_rscripts_duration

#!/bin/sh
MODS=$*

function get_section() {
  local result

  result=$(cat ${SECTIONS}/.${1})
  if [ ! -z "${result}" ]; then
    result="-s .${1} ${result}"
  else
    result=""
  fi
  echo ${result}
}

if [ -z "${MODS}" ]; then
  MODS=$(cat /proc/modules | cut -d" " -f1)
fi

for MOD in ${MODS}; do
  SECTIONS=/sys/module/${MOD}/sections

  TEXT=$(get_section "text")
  DATA=$(get_section "data")
  BSS=$(get_section "bss")

  if [ ! -z "${TEXT}${DATA}${BSS}" ]; then
    echo "${MOD}.ko ${TEXT} ${DATA} ${BSS}"
  fi
done

#!/bin/bash
#_PP_DEBUG=1
#_DEBUG_TO_FILE=1

# abspath but keep symbollink
abspath() {
    for p in "$@"; do
        if ! printf '%s' "$p" | grep -q -- '^/'; then
            realpath -se "$PWD/$p"
        else
            realpath -se "$p"
        fi
    done
}

function _msg() {
    _PP_RED_CL="\\033[0;${1}m"
    _PP_NC='\033[0m' # No Color==
    _new_line=$2
    shift 2
    [[ $_new_line == 'y' ]] && echo -e "${_PP_RED_CL}$@${_PP_NC}"
    [[ $_new_line == 'n' ]] && echo -e  -n "${_PP_RED_CL}$@${_PP_NC}"
    return 0
}

function normal() { _rep=$1; shift; for i in $(seq 1 $_rep); do echo "${@:1}"; done }
function normaln() { echo -n "$@"; } #normal without linebreak
function bold() { _msg 1 'y' "$@"; } #bold
function boldn() { _msg 1 'n' "$@"; } #bold
function warning() { _msg 31 'y' "$@"; } #RED
function warningn() { _msg 31 'n' "$@"; } #RED
function success() { _msg 32 'y' "$@"; } #GREEN
function successn() { _msg 32 'n' "$@"; } #GREEN
function alert()  { _msg 33 'y' "$@"; } #YELLOW
function alertn()  { _msg 33 'n' "$@"; } #YELLOW

function _dpp() {
    if [[ -n $_PP_DEBUG  || $_PP_DEBUG -eq 1 ]]; then
        if [[ -z $_DEBUG_TO_FILE ]]; then
            echo -e "${@}"
        else
            echo -e "${@}" >> /tmp/_patch_debug.log
        fi
    fi
}

function _dpp_init() {
    if [[ ( -n $_PP_DEBUG  || $_PP_DEBUG -eq 1 ) && -n $_DEBUG_TO_FILE ]]; then
        echo "" > /tmp/_patch_debug.log
    else
        rm -f /tmp/_patch_debug.log
    fi
}

_dpp_init

#get file modify time y-m-d:h:m:s
function file_timestamp() {
    echo "$(date -r $1 '+%Y-%m-%d:%H:%M:%S')"
}

function create_version_file() {
   _version_file=$1
   _bsp_version=$2
   echo "###!ATTENTION!!-AUTO GENERATED FILE-NO MANUAL CHANGE IN ANY CASES!!! ###" >$_version_file
   echo "###!This file can only be overwritten by BRCM WIFI patching package  ###" >>$_version_file
   echo "" >>$_version_file
   echo "##### PATCH VERSIONS #####" >>$_version_file
   [[ -n $_bsp_version ]] && echo "BSP:${_bsp_version}" >>$_version_file
}

# input version in BSP/WIFI/RDKv5s6 and do not use version with all 0 such as BSPv0W0 will not be used
# the simplified version will be used in running image version display
function simple_version() {
    _vers=$(echo $1 |sed 's/^-//g')
    _vers=(${_vers//-/ })
    _version_str=""
    for _patch_str in ${_vers[@]}
    do
        [[ -n  $(echo $_patch_str|sed -Ee "s/([1-9]+)/\1/p") ]] && _version_str+="-$_patch_str"
    done
    _version_str=${_version_str/-/}
    _version_str=$(echo $_version_str|sed -Ee 's/((BSPv[1-9][0-9]*W([1-9][0-9]*)-WIFIv[0-9]+)B\3)(.*)/\2\4/g')
    _version_str=$(echo $_version_str|sed -Ee 's/BSPv[0-9]W([1-9][0-9]*)-(WIFIv[0-9]+B\1.*)/\2/g')
    _version_str=$(echo $_version_str|sed -Ee 's/(BSPv[0-9]+W[1-9][0-9]*)-(:?WIFIv0|WIFIv0-)(.*)/\1\3/g')
    _version_str=$(echo $_version_str|sed -E -e 's/c0|B0|W0|S0//g')
    _version_str=$(echo $_version_str|sed -Ee 's/(.*)-(RDKv0)/\1/g')
    _version_str=$(echo $_version_str|sed -Ee 's/^BSPv0-(.*)/\1/g')
    echo $_version_str|sed -Ee 's/WIFIv0-(.*)/\1/g'|sed -Ee 's/^-*//'|sed -Ee 's/-*$//'

}


# $1=patch_file
# $2=top_patch_dir
function patch_files_missing() {
    g_print_patch="CHECKMISSING"
    get_patch_files $1 $(find_patch_level  $1 $2) $2
    unset g_print_patch
}
function get_patch_files() {
    _files=($(awk '/^[+]{3} \S+|^[-]{3} \S+/{printf("%s%s ",$1,$2)}' $1))
    _patch_level=$2
    _top_patch_dir=$3
    for i in $(seq 0 $((${#_files[@]}/2-1))); do
        _filepair=(${_files[$((2*i))]} ${_files[$((2*i+1))]})
        _file1=${_filepair[0]//---}
        _file2=${_filepair[1]//+++}
        _symbol="Modified"
        _file=$_file2
        [[ $_file1 =~ .*/dev/null ]] && _symbol="Add    "
        [[ $_file2 =~ .*/dev/null ]] && _symbol="Removed" && _file=$_file1
        dirs=($(echo $_file|tr '/' ' '))
        _file="$(echo ${dirs[@]:$_patch_level})"
        _file="${_file// //}"
        _file=${_file/.\//}
        # for BSP and BSPWIFI, _top_patch_dir is empty #
        # for WIFI, it is relavive to g_top_root       #
        # Otherwize it is RDKB SOC/SYSTEM              #
        if  [[ -n $_top_patch_dir ]]; then
             if [[ $_top_patch_dir =~ bcmdrivers/broadcom/net.* ]]; then
                 _delimeter=$_top_patch_dir/
            else
                 _delimeter="../$(basename $_top_patch_dir)/"
            fi
        fi
        if [[ -n $g_print_patch ]]; then
            [[ -z $_delimeter ]] && _delimeter="./"

            if [[ $g_print_patch =~ ALL || ! $_symbol =~ Removed ]];then
                if [[ $g_print_patch =~ CHECKMISSING ]]; then
                    [[ ! -f $_top_patch_dir/${_file}  && ! $_symbol =~ Add.* ]] && echo "${_file}"
                else
                    echo -n "${_delimeter}${_file} "
                fi
            fi
        else
            print_file_list "$_symbol" "${_delimeter}${_file}"
        fi
    done
}

function find_patch_level() {
    pushd $2 2>&1>/dev/null
    local _files=($(awk '{ if(match($0,"^([+][+][+]|---)\\s")) {print $2}}' $1|sort))
   _files=(${_files[@]//\/dev\/null/})
   depth=-1
   if [[ -n $_files ]]; then
      local dir
      files=${files/./}
      local dirs=($(echo $_files|tr '/' ' '))
      for dir in ${dirs[@]}
      do
         depth=$((depth+1))
         [[ $(find ./ -maxdepth 1 -type d -name $dir) ]] && break
      done
   else
      depth=-2
   fi
   popd 2>&1>/dev/null
   echo $depth
}

#######back-compatible-to restore previous version of patching source to new style

function transfer_wifi_patches_version() {
    #input is wifi patch numbers, assuming all the patches are in new patches. This should be true
    #if has wifi patched, then mark it.
    _dpp $LINENO ${FUNCNAME[0]}
    [[ $1 -le 0 ]] && return
    _patched_wifi_number=$1
    _files=($(find $g_wifi_patch_src_dir/$g_wifi_version/ -maxdepth 1 -type f  -name "*.patch"|sort))
    _patch_files=()
    for _pf in ${_files[@]}; do
        _nums=$(echo $(basename $_pf)|sed -Ee 's|([0-9]+).*patch|\1|')
        _nums=($(expr $_nums + 0 ))
        if [[ $(pp_patches_count $_pf) -gt 1 ]]; then
            _nums=($(echo $(basename $_pf)|sed -Ee 's/^([0-9]+)-([0-9]+)-rb.*.patch/\1 \2/g'))
            _nums=($(seq ${_nums[0]} ${_nums[1]}))
        fi
        [[ ${_nums[-1]} -le $_patched_wifi_number && ${_nums[0]} -le $_patched_wifi_number ]] && \
            touch $_pf.done && _patch_files+=($_pf)
    done
    [[ ${#_patch_files[@]} -gt 0 ]] && pp_patch_version_modify_wifi $g_wifi_patch_src_dir/$g_wifi_version/readme.txt $g_top_dir/patch.list.bspwifi ${_patch_files[@]}
}

function last_patched() {
   _patch_version_file=$g_current_patch_version
   [[ -n $1 ]] && _patch_version_file=$1
   if [[ -n $_patch_version_file ]]; then
      _last_patched=($(awk '/^Patched:.*patches-.*cpe5.*.tgz.*/{print $1}' $_patch_version_file))
      if [[ -n $_last_patched ]]; then
         echo $_last_patched|sed -En 's/^Patched:.*patches-(.*)[.-]cpe5.*/\1/p'
      fi
   fi
}

function transfer_patch_version() {
    _dpp $LINENO ${FUNCNAME[@]}
    _patch_version_file="$g_current_patch_version"
    #if new version file exists already(migrated already) or wifi doesn't exist[only wifi used previous    # patch mechanism.
    [[ -f $_patch_version_file || ! -d $g_wifi_impl_dir ]] && return 0
    _epivers_hdr=$(find $g_wifi_impl_dir/main/ -name "epivers.h")
    [[ -z $_epivers_hdr ]] && return 0 # only for release source. It is needed

    _version=($(awk '/^\s*#define\s+EPI_VERSION_STR/{print $5}' $_epivers_hdr))
    _wifi_version=($(awk '/^\s*#define\s+EPI_VERSION_STR/{print $3}' $_epivers_hdr))
    _wifi_version=${_wifi_version/\"}
    _wifi_version=${_wifi_version//./_}
    if [[ -n $_version ]]; then
        #release source with previous patch applied, we need construct "brcm_patch_version" file
        sed -i 's/-PvB[0-9*.c]\+W[0-9*_c]*//g' $_epivers_hdr
        sed -i 's/Bm[0-9_,.c]\+//g' $_epivers_hdr
        sed -i 's/Wm[0-9,.c]\+//g' $_epivers_hdr
        sed -i 's/-BSPv[0-9]\+c[0-9]\+s[0-9]\+//g' $_epivers_hdr
        sed -i 's/-BSPv[0-9]\+//g' $_epivers_hdr
        _vers=($(echo ${_version}|sed -En 's/.*PvB([0-9]+).W([0-9]+).*/\1 \2/p'))
        _bsp=($(echo ${_version}|sed -En 's/.*BSPv([0-9]+).*/\1/p'))
        [[ -z $_bsp ]] && _bsp=0
        _rdkc=($(echo ${_version}|sed -En 's/.*BSPv[0-9]+c([0-9]+).*/\1/p'))
        [[ -z $_rdkc ]] && _rdkc=0
        _rdks=($(echo ${_version}|sed -En 's/.*BSPv.*s([0-9]+).*/\1/p'))
        [[ -z $_rdks ]] && _rdks=0
        _bsp_version="BSPv${_bsp}W${_vers[0]}"
        __wifi_version="WIFIv${_vers[1]}B${_ver[0]}"
        [[ ! -f $_patch_version_file ]] && create_version_file $_patch_version_file $_bsp_version
        _wifi_version_str="WIFIv${_vers[1]}B${_vers[0]}"
        transfer_wifi_patches_version ${_vers[1]}
        echo "WIFI-$_wifi_version:$_wifi_version_str $_wifi_version_str" >>$_patch_version_file
        if [[ $_rdks -gt 0 && $_rdkc -gt 0 ]]; then
            local _rdk_version="RDKv${_rdkc}S${_rdks}"
            echo "RDK:$_rdk_version $_rdk_version" >>$_patch_version_file
        fi
        _wifi_version=${_wifi_version//_/.}
        rm -f $g_top_dir/wifi_patch_version.$_wifi_version

        print_file_list "Removed" "wifi_patch_version.$_wifi_version"
        print_file_list "Added" "brcm_patch_version"
    fi
}

function previous_patch_migration() {
    ## migration from previous patch mechanism to current new one
    ## no epivers.h modfiication any more
    _dpp $LINENO ${FUNCNAME[0]}
    [[ -n $g_wifi_impl_dir ]] && transfer_patch_version
    _dpp $LINENO "previous_patch_migration"

    if [[ -f $g_new_patch_version && ! -f $g_current_patch_version ]]; then
        cp  -f $g_new_patch_version  $g_current_patch_version
        sed -i '/^Patched.*/d' $g_current_patch_version
    fi
    [[ ! -f $g_current_patch_version ]] && create_version_file $g_current_patch_version
}
##########################END of Back Compatible ##################################


function print_file_list() {
    if [[ -n $g_patches_filelist ]]; then
        printf "%-14s: %s\n" $1 $2 >> $g_patches_filelist
    else
        printf "%-14s: %s\n" $1 $2
    fi
}

# input : list of patches Or
#        : patch dir
# output: last patch number
# attetnion: please no extra echo debug in this function
function get_last_patch_num() {
    if [ $# -gt 0 ]; then
        local patch_name=""
        local patch_files=""
        if [[ ! $1 =~ .*\.patch$ &&  -d  $1 ]];then
            if [[ -n $2 ]]; then
                patch_files=($(find $1/ -maxdepth 1 -regex '.*/c?[0-9]+[^/]*\.patch$'|sort))
            else
                patch_files=($(find $1/ -maxdepth 1 -regex '.*/[0-9]+[^/]*\.patch$'|sort))
            fi
            if [[ ! -z $patch_files ]];then
                patch_name=${patch_files[-1]}
            fi
        else
            patch_name="${@:$#}"
        fi
        if [[ -n $patch_name ]]; then
            patch_name=$(basename $patch_name)
            #get second number first
            local num=$(echo $patch_name |sed -n "s/^[0-9]\+-0*\([1-9][0-9]*\)[_-].*/\1/p")
            if [ ! -z "$num" ];then
                echo $num
                return 0
            else
                if [[ -n $2 ]]; then
                    num=$(echo $patch_name |sed -n "s/^c\?0*\([1-9][0-9]*\)[_-].*/\1/p")
                else
                    num=$(echo $patch_name |sed -n "s/^0*\([1-9][0-9]*\)[_-].*/\1/p")
                fi
                if [ ! -z "$num" ];then
                    echo $num
                    return 0
                fi
            fi
        fi
    fi
    echo "0"
}

#input "result options"
function choose_from_options() {
    [[ $# -lt 3 ]] && return
    local options=()
    for _p in "$@"; do
        options+=("$_p")
    done
    local _selection_=0
    if [[ ${#options[@]} -eq 3 ]]; then
        eval $1="${options[2]}"
    else
        echo ${options[1]}:
        echo "=================================="
        for option in "${options[@]:2:${#options[@]}}"
        do
            echo "${_selection_} : $option"
            _selection_=$((_selection_+1))
        done
        echo "================================="
        echo -n "choose:"
        read  _selection_
        while [[ $_selection_ =~ ^[0-9]+$ && $_selection_ -gt $((${#options[@]}-2)) ]];
        do
            echo -n "invalid, choose:"
            read  _selection_
        done
        if [[ $_selection_ =~ ^[0-9]+$ ]] && (($_selection_ <= (${#options[@]}-2) )); then
            echo ${options[((_selection_+2))]}
            eval $1="${options[$((_selection_+2))]}"
            return 0
        else
            return 1
        fi
    fi
}

function _get_input_() {
    local input="__init_value"
    _bold=$(tput bold)
    _normal=$(tput sgr0)
    if [[ $# -gt 2 ]];then
        echo -n "${_bold}$1($2):${_normal}"
    else
        echo -n "${_bold}$1():${_normal}"
    fi
    read input
    if [[ -n $input ]]; then
        if [[ -z $3 ]]; then
            eval "$2=\"$input\""
        else
            eval $3="$input"
        fi
    fi
}

function patch_dir_update() {
    local patch_dir=$1
    local patch_name=$2
    local patch_file=$3
    mkdir -p $patch_dir
    [[ -z $3 ]] && patch_file=$patch_name
    local last_num=$(get_last_patch_num $patch_dir)
    last_num=$(printf %04d $((${last_num}+1)))
    cp  $patch_file $patch_dir/${last_num}-$(basename $patch_name)
    [[ -z $g_description ]] && _get_input_ "Patch description:" g_description
    echo "- {csp: CS${g_csp_num}, ref: SWBCACPE-${g_jira_num}, chg: ${g_changelist_num} } -# ${g_description}">>${patch_dir}/patch.list
    success "$patch_dir/${last_num}-$(basename $patch_name) is created"
}


function get_files_inpatch() {
    local rbfile=$1
    local files=($(awk '{ if (match($1,"\\+\\+\\+")) { print $2 }}' $rbfile))
    echo "${files[@]}"
}

function find_file_in_dir() {
   local impl_dir=$(abspath $1)
   local rbfile=$2
   _type="f"
   [[ -n  $3 ]] && _type=$3
   local file_name=$(basename $rbfile)
   local found=($(find $impl_dir/ -type $_type -name $file_name))
   g_found_result=""
   if [[  ${#found[@]} -gt 0 ]]; then
      rbfile=$(echo "$rbfile" |sed -e 's@KUDU[^/]*/\|proj/\|branches/\|products/\|trunk/@@g')
      match_files=()
      for file in ${found[@]}
      do
         [[ $file =~ .*$rbfile ]] && match_files+=($file)
      done
      [[ -z $match_files ]] && match_files=(${found[@]})
      if [[ -z $match_files ]]; then
         g_found_result=""
      else
         if  [[ ${#match_files[@]} -gt 1 ]]; then
            echo "$rbfile is in:"
            local item=0
            for part in ${found[@]}
            do
               echo "$item:$part"
               item=$((item+1))
            done
            echo "which one(s) to patch:"
            read select
            if [[ ! -z $select ]]; then
                selects=($select)
                founds=()
                for _select in ${selects[@]}
                do
                    if [[ -d ${found[$_select]} ]]; then
                        founds+=($(find ${found[$_select]}/ -type f -name $file_name))
                    else
                        founds+=($(find $(dirname ${found[$_select]})/ -type f -name $file_name))
                    fi
                done
                g_found_result="${founds[@]}"
            fi
         else
            g_found_result=$match_files
         fi
      fi
   fi
}

function patch_files_d2u_convert() {

    local impl_dir=$(abspath -s $1)
    local patchfile=$(abspath $2)
    local files=($(get_files_inpatch $patchfile $files))
    if [[ ! -z $files ]]; then
        for fl in ${files[@]}
        do
            find_file_in_dir $impl_dir $fl >/dev/null
            local result="$g_found_result"
            if [[ ! -z $result ]]; then
                dos2unix -q $result
            fi
        done
    fi
}
# return patch package's last bsp and wifi version number in file wifi_patch_version.txt
# Parameters:
#    in: top_dir
#    in: wlimpl "impl55"
#    out[optional]: last bsp number
#    out[optional]: last wifi numberh

function get_pp_last_ver() {
    if [[ $# -gt 1 ]]; then
        local top_dir=$1
        local wlimpl=$(basename $2)
        local wifi_version=""
        if [ $wlimpl == "impl00" ]; then
            wifi_version="00"
        else
            wlimpl_get_ver $top_dir/bcmdrivers/broadcom/net/wl/$wlimpl  wifi_version
        fi
        if [[ -f $top_dir/wifi_patch_version.$wifi_version ]]; then
            local src_ver="$(cat $top_dir/wifi_patch_version.$wifi_version)"
            local src_last=($(echo $src_ver|sed 's/.*PvB\([0-9_c]\+\)W\([0-9c]\+\).*/\1 \2/'))
            if [ ${#src_last[@]} -eq 2 ];then
                if [[ ! -z $3 ]]; then
                    local tmp=${src_last[0]}
                    tmp=(${tmp/c/ } 0)
                    eval "${3}=(${tmp[@]})"
                    tmp=${src_last[1]}
                    tmp=(${tmp/c/ } 0)
                    eval "${4}=(${tmp[@]})"
                    return 0;
                else
                    echo "${src_last[0]} ${src_last[1]}"
                fi
            fi
        fi
        if [[ ! -z $3 ]]; then
            eval "${3}=(-1 0)"
            eval "${4}=(-1 0)"
        else
            echo "-1 0 -1 0"
        fi
    else
        echo "get_pp_last_ver topdir wlimpl last_bsp_num[out] last_wifi_num[out]"
    fi
}

#return a patch files version number,patches all in 0xxx-0xxx-yyy.patch
#Parameters:
#   in:patch name
#   out:array of patch_number[at most two]

function get_patch_number() {
    local patch_name="$1"
    patch_name=$(basename $patch_name)
    #get second number first
    local nums=($(echo $patch_name |sed -n "s/^0*\([0-9]\+\)-0*\([1-9][0-9]*\)[_-].*/\1 \2/p"))
    if [ ${#nums[@]} -eq 2 ]; then
        echo $(seq ${nums[0]} 1 ${nums[1]})
    else
        num=$(echo $patch_name |sed -n "s/^[c]\?0*\([1-9][0-9]*\)[_-].*/\1/p")
        if [[ ! -z $2 ]]; then
            eval "${2}=($num)"
        fi
        echo "$num"
    fi
}

# wifi version string is in format xx_xx_xx_xx
# for example 17_10_0_0 /17_10_157_2805
# this function append _0 if not in four number format,like branch 17_10 has version 17_10_0_0
function fix_wifi_version() {
    local name=$1
    local names=(${name//_/ })
    local nums=$((${#names[@]}))
    for i in $(seq $nums 3)
    do
        name="${name}_0"
    done
    echo $name
}

#return an array to dindicate wifi release version and it is release/internal source
#Parameter:
#    in: wlimpl_dir
#   out: (relver dirwithpatch wifi_source_ver wlimpl)
#        (17_10_188 internal|release KUDU_REL_xxxx impl81)

function wlimpl_get_ver() {
    if [[ $# -gt 1 ]]; then
        local wlimpl_dir=$(abspath $1)
        if [[ -e $wlimpl_dir/main ]]; then
            local epivers_hdr=$(find $wlimpl_dir/main/ -name "epivers.h")
            local release_ver=""
            local wifi_source_ver=""
            local patch_dir="internal"
            if [  -z "$epivers_hdr" ];then
                epivers_hdr=$(find $wlimpl_dir/main/ -name "epivers.sh")
                if [ ! -z "$epivers_hdr" ];then
                    local release_ver_line="$(awk '/^\s*SVNURL.*KUDU((_REL_)|(_BRANCH_)|(_TWIG_)).*\/src.*/{print}' $epivers_hdr)"
                    release_ver="$(echo $release_ver_line |sed 's/.*KUDU_[TRB][^0-9]*_\([0-9_]*_[0-9]\+\).*/\1/')"
                    wifi_source_ver="$(echo $release_ver_line |sed 's/.*\(KUDU_[TRB][^0-9]*_[0-9_]*_[0-9]\+\).*/\1/')"
                fi
            else
                local release_ver=$(awk '/.*EPI_VERSION_STR/{print $3; exit}' $epivers_hdr)
                if [ ! -z $release_ver ];then
                    release_ver=${release_ver//./_}
                    release_ver=${release_ver//\"/}
                fi
                patch_dir="release"
                wifi_source_ver="KUDU_REL_${release_ver}"
            fi
            #echo "$release_ver" "$patch_dir"
            release_ver=$(fix_wifi_version $release_ver)
            eval "${2}=($release_ver $patch_dir $wifi_source_ver)"
        fi
    else
        echo "wlimpl_get_ver wlimpldir verresult"
    fi
}

function create_epibsppatch_header() {

    [[ $g_brcm_list_patches_only -eq 1 || $g_brcm_patching_status -ne 0 ]] && return 1
    _simple_version=$(patch_version_strs $g_current_patch_version 1 1)
    echo  'IMAGE_VERSION="${IMAGE_VERSION}('"$_simple_version"')"' >$g_top_dir/targets/.patch.version
    [[ $g_has_wifi -ne 1 || -z $g_wifi_impl_dir ]] && return 0
    #for wifi, to create epibsppatch.h under wifi directory
    if [[ $g_internal_build -ne 0 ]]; then
        _epivers_hdr=($(find $g_wifi_impl_dir/ -name "epivers.h.in"));
    else
        _epivers_hdr=($(find $g_wifi_impl_dir/ -name "epivers.h"));
    fi
   for _hdr_file in ${_epivers_hdr[@]}
   do
      #back compatible, in case wifi soure is still using old version, update it to append BSP version
      if [[ -z $(grep "EPI_BSP_PATCH" $_hdr_file) && -n $(grep "EPI_CUSTOM_VER" $_hdr_file) ]]; then
         name="#define EPI_BSP_PATCH \"\"\n#if defined __has_include\n#  if __has_include \(\"epibsppatch.h\"\)\n#    include \"epibsppatch.h\"\n#  endif\n#endif /* EPI_BSP_PATCH */\n"
         print_file_list    "Modified"    $_hdr_file
         sed -E -i "/^#endif.*EPI_CUSTOM_VER.*/a$name" $_hdr_file
         sed -E -i 's/EPI_CUSTOM_VER$/EPI_CUSTOM_VER EPI_BSP_PATCH/' $_hdr_file
      fi
   done

    [[ $g_internal_build -ne 0 ]] && _simple_version="(I)$_simple_version"
    #new way of apply BSP patch
    for _hdr_file in ${_epivers_hdr[@]}
    do
        _epibsppatch_header="$(dirname $_hdr_file)/epibsppatch.h"
        if [[ -f $_epibsppatch_header ]]; then
            _ver=$(awk '{if  (match($0,"^#define EPI_BSP_PATCH .*")) {print $4}}' $_epibsppatch_header)
            [[ -n $_ver && ${_ver//\"/} == $_simple_version ]] && _simple_version=""
        fi
        [[ -z $_simple_version ]] && continue
        _dpp $LINENO "update/create $_epibsppatch_header"
        echo "//Dynamic generated patch version file, don't need to be in version control">$_epibsppatch_header
        echo "#ifdef EPI_BSP_PATCH">>$_epibsppatch_header
        echo "#undef EPI_BSP_PATCH">>$_epibsppatch_header
        echo "#endif">>$_epibsppatch_header
        echo "#define EPI_BSP_PATCH \" $_simple_version\"">>$_epibsppatch_header
    done
}


function patch_dir_with_patches() {
   local dest_dir=$1
   local is_revert=$2
   local is_test=$3
   shift 3
   local patch_files=(${@})
   local patch_num=0
   local revert_patch=""
   [[ $is_revert -eq 1 ]] && revert_patch="-R"
   _top_patch_dir="$(echo $dest_dir|sed "s|$g_top_dir||")"
   _top_patch_dir=${_top_patch_dir/\//}
   _top_patch_dir=${_top_patch_dir%/%}
   _dpp "$LINENO:patch_dir_with_patches:\n    dest_dir:$dest_dir\n is_revert=$is_revert"
   pushd $dest_dir >/dev/null
   for pf in ${patch_files[@]}
   do
      _dpp -e "$LINENO:patch_dir_with_patches: $pf"
      _clnum=$(echo $(basename $pf)|sed -Ee 's|.*-(cl[0-9]+)-SWBCACPE.*patch|\1|')
      if [[ -n $_clnum ]]; then 
          #for BSP changes, we will do some pre-required actions if any
          _operations=($( awk -v pattern="^#@.*" -F "@"  '{ if (match($0,pattern)) {printf("%s %s ", $3, $4) }}'  $pf))
          if [[ ${#_operations[@]} -gt 1 ]]; then
             for i in $(seq 1 2 $((${#_operations[@]}))); do
                 _action=${_operations[$((i-1))]}
                 _ofield=${_operations[$i]}
                 case $_action in 
                     NEWFILE_REMOVE_FIRST)
                         _newfiles=($(awk '{ if (match($0,"^--- /dev/null")) { getline; print $2; }}' $pf))
                         for f in ${_newfiles[@]};
                         do
                             if [[ $g_brcm_patching_dryrun -eq 1 ]]; then
                                 if [[ $is_revert -eq 1 ]]; then 
                                     mv -f $f.dryrun $f
                                 else
                                     mv -f $f $f.dryrun
                                 fi
                             else
                                     rm -f $f.dryrun
                                     rm -f $f
                             fi
                         done
                         ;;
                 esac
             done
          fi
      fi
      [[ "$pf" == *"D2UCONVERT"* ]] && patch_files_d2u_convert $dest_dir $pf

      if [[ ( $is_revert -eq 0 && ! -e $pf.done  && ! -e $pf.rdone ) || $is_revert -eq 1 ]]; then
         patch_num=$((patch_num+1))
         local patch_level=$(find_patch_level $pf $dest_dir)
         [[ $patch_level -eq -2 ]] && touch "$pf.done" && continue #empty patch file
         [[ $patch_level -lt 0 ]] && warning "$pf in wrong format??" && continue
         if [[ -n $(grep "GIT binary patch" $pf) ]]; then
            git apply $revert_patch -p$patch_level <$pf
            touch "${pf}.done"
         elif  patch -p$patch_level -N $revert_patch --dry-run --silent <$pf 2>&1 >/dev/null; then
            patch -p$patch_level $revert_patch <$pf 2>&1 >/dev/null
            [[ $g_brcm_patching_dryrun -eq 0  && $is_test -eq 0 ]] && printf ".."
            if [[ $is_revert -eq 0 ]]; then touch "${pf}.done"; else  rm -f "${pf}.done"; fi
            [[ $g_brcm_patching_dryrun -ne 1 ]]  && get_patch_files $pf $patch_level $_top_patch_dir
         else
            local _failure=$(patch -p$patch_level -N $revert_patch --dry-run <$pf 2>&1)
            if [[ $_failure  =~ .*Reversed.*patch[[:space:]]detected.* ]]; then
               [[ $is_revert -eq 0 && $is_test -eq 0 ]] && touch "${pf}.done"
               [[ $is_revert -eq 0 && $is_test -eq 1 ]] && touch "${pf}.norevert"
               alert "$(basename $pf) seems patched"
           elif [[ $is_test -eq 1 ]]; then
              local _patch_failure_log_file="$g_patch_src_root/patch_failure-$(basename $pf).log"
              rm -f $_patch_failure_log_file
              warning "patching:$pf failure, log at:"
              echo $_patch_failure_log_file
              echo "patch file:$pf--">>$_patch_failure_log_file
              echo "Failure:$_failure">>$_patch_failure_log_file
              popd >/dev/null
              return $patch_num
            fi
         fi
      fi
   done
   popd >/dev/null
   return 0

}

function pp_patch_version_modify() {
    _src_patch_version=$1;
    _dst_dir_version=$2
    _patch_type=$3
    _patch_files=($(find $4/ -maxdepth 1 -type f -regex ".*patch$"|sort))
    [[ ! -f $_src_patch_version ]] && warning "no $_src_patch_version??" && return -1
    touch $_dst_dir_version
    for pf in ${_patch_files[@]};
    do
        _jira=$(echo $(basename $pf)|sed -Ee 's|.*(SWBCACPE-[0-9]+).*patch|\1|')
        if [[ -n $_jira ]]; then
            _jira_strs="$(awk '{if ($5=="'$_jira',") {print $0}}' $_src_patch_version)"
            _jira_strs_dst="$(awk '{if ($5=="'$_jira',") {print $0}}' $_dst_dir_version)"
            if [[ -n "$_jira_strs" ]]; then
                if [[ -z "$_jira_strs_dst" ]]; then
                    if [[ $_patch_type == "BSPWIFI" ]]; then
                        readarray -t _jira_items <<< "$_jira_strs"
                        _has_history=$(awk '/##### BSPWIFI.*/{print $0}' $_dst_dir_version)
                        [[ -z $_has_history ]] && echo "##### BSPWIFI patches ######" >> $_dst_dir_version
                        for i in $(seq 0 $((${#_jira_items[@]}-1))); do
                            _mt=$(awk -v mt="${_jira_items[$i]}" '$0 ~ mt' $_dst_dir_version)
                            [[ -z $_mt ]] && sed  -i '/^##### BSPWIFI patches.*/a '"${_jira_items[$i]}" $_dst_dir_version
                        done
                    else
                            echo "$_jira_strs" >>$_dst_dir_version
                    fi
                fi
            else
                alert "!!!! there is no release version in $_src_patch_version for $_jira"
            fi
        fi
    done
}

function pp_patch_version_modify_wifi() {
    _src_patch_version=$1;
    [[ ! -f $_src_patch_version ]] && warning "no $_src_patch_version??" && return -1
    _dst_dir_version=$2
    if [[ -d $3 ]]; then
        _patch_files=($(find $3/ -maxdepth 1 -type f -regex ".*patch$"|sort))
    else
        shift 2
        _patch_files=(${@})
    fi
    touch $_dst_dir_version
    for pf in ${_patch_files[@]}; do
        _nums=$(echo $(basename $pf)|sed -Ee 's|([0-9]+).*patch|\1|')
        _nums=($(expr $_nums + 0 ))
        if [[ $(pp_patches_count $pf) -gt 1 ]]; then
            _nums=($(echo $(basename $pf)|sed -Ee 's/^([0-9]+)-([0-9]+)-rb.*.patch/\1 \2/g'))
            _nums=($(seq ${_nums[0]} ${_nums[1]}))
        fi
        for _num in ${_nums[@]}
        do
            _has_history=$(awk '/##### WIFI Patches.*/{print $0}' $_dst_dir_version)
            [[ -z $_has_history ]] &&  echo "##### WIFI Patches #####">>$_dst_dir_version

            _num_str=$(printf "%04d" $_num)
            _rb_items=$(awk "/$_num_str.*/"'{print $0}' $_src_patch_version)
            _rb_items="$g_wifi_version: $(echo "$_rb_items"|tr '\n' ' ')"
            _rb_items=${_rb_items//$_num_str:/}
            _match_pattern="$(echo $_rb_items|awk '{printf("%s %s",$1,$2)}')"
            [[ -z $(grep "$_match_pattern" $_dst_dir_version) ]] && \
                sed -i '/^##### WIFI Patches.*/a '"$_rb_items" $_dst_dir_version
        done
    done
}


# count patche's number of 000x-000y-rbxxxx.patch, avoid it in the future, back compatible  */
function pp_patches_count(){
    _patches=($@)
    _count=0
    for _patch in ${_patches[@]}
    do
        _count=$((_count+1))
        _numbers=($(echo $(basename $_patch)|sed -Ee 's/^([0-9]+)-([0-9]+)-rb.*.patch/\1 \2/g'))
        [[ ${#_numbers[@]} -eq 2 ]] && _count=$((_count+${_numbers[1]}-${_numbers[0]}))
    done
    echo $_count
}

# function to patch destination dir with patches from the source dir
# Parameters:
#    in: dest_dir
#   in: src_dir
# Result:
#   .done file is created when patch success
#   .failed file is created when failed to patch

function patch_dir_from_dir() {
    local dest_dir=$1
    local src_dir=$2
    local is_revert=$3
    local is_test=$4
    _dpp "$LINENO:patch_dir_from_dir:\n    dest_dir:$dest_dir\n    src_dir:$src_dir\n    is_revert=$is_revert"

    if [[ -d $dest_dir  && -d $src_dir ]]; then
        local patch_files=($(find $src_dir/ -maxdepth 1 -type f -regex ".*patch$"|sort))
        local patch_files_done=($(find $src_dir/ -maxdepth 1 -type f -regex ".*patch\.[r]?done$"|sort))
        for fld in ${patch_files_done[@]}
        do
            fld=${fld%.*done}
            patch_files=(${patch_files[@]/$fld})
            if [[ ${#patch_files[@]} -eq 1 && $patch_files == $fld ]]; then
                patch_files=()
            fi
        done
        local allfiles=(${patch_files[@]})
        if [[ $g_brcm_list_patches_only -eq 1 ]]; then
            if [[ ${#patch_files[@]} -gt 0 ]]; then
                [[ $g_brcm_patching_dryrun -eq 1 && $5 =~ ^g_new_patch_BSP_num$ ]] && bold "\n== BSP Patches ==="
                [[ $g_brcm_patching_dryrun -eq 1 && $5 =~ ^g_new_patch_BSPWIFI_num$ ]] && bold "\n== BSPWIFI Patches ==="
                [[ $g_brcm_patching_dryrun -eq 1 && $5 =~ ^g_new_patch_RDKBSYS_num$ ]] && bold "\n== RDKB SYSTEM Patches ==="
                [[ $g_brcm_patching_dryrun -eq 1 && $5 =~ ^g_new_patch_RDKBSOC_num$ ]] && bold "\n== RDKB SOC Patches ==="
                [[ $g_brcm_patching_dryrun -eq 1 && $5 =~ ^g_new_patch_WIFI_num$ ]] && bold "\n== WIFI Patches ==="
                if [[ $g_brcm_patching_dryrun -eq 1 ]]; then
                    for _tpf in ${allfiles[@]};
                    do
                        _missing_files=($(patch_files_missing  $_tpf $dest_dir))
                        if [[ -n $(patch_files_missing  $_tpf $dest_dir ) ]]; then
                            echo  "$_tpf [NO_SRC]"
                            alert "=== Missing Src files [Feature-based files, you may have no such feature(s)] ==="
                            for _mfl in ${_missing_files[@]};
                            do
                            alert "    $_mfl "
                            done
                        else
                            echo $(echo $_tpf|sed "s|$g_top_dir|.|")
                        fi
                    done
                fi
                g_new_patch=$(($g_new_patch+${#allfiles[@]}))
            fi
            return 0
        fi
        local ret_code=0
        #now get all the patches needs to be patched, try to patch

        pushd $g_top_dir 2>&1 >/dev/null
        local _missing_num=0
        for _tpf in ${allfiles[@]};
        do
            if [[ -n $(patch_files_missing  $_tpf $dest_dir ) ]]; then
                rm -f $_tpf
                g_patch_incomplete=1
                _missing_num=$((missing_number+1))
                patch_files=(${allfiles[@]/$_tpf})
            fi
        done
        popd 2>&1 >/dev/null
        [[ -n $6 ]] && eval "$6=$(($6+$_missing_num))"

        if [[ ${#patch_files[@]} -eq 0 ]]; then
            ret_code=0
        else
            patch_dir_with_patches $dest_dir $is_revert $is_test ${patch_files[@]}
            ret_code=$?
            if  [[ ! -z $5 ]] ; then
                _count=$(pp_patches_count ${patch_files[@]})
                eval "$5=$(($5+$_count))"
                g_new_patch=$(($g_new_patch+${#patch_files[@]}))
            fi
        fi

        [[ $g_brcm_patching_status -eq 0 ]] && g_brcm_patching_status=$ret_code

        if [[ $is_test -eq 1 && $is_revert -eq  0  && -z $g_brcm_no_revert ]]; then
            #if it is test, anyway need to revert all patched  files
            if [[ $ret_code -eq 0 ]]; then
                local revert_patches=($(echo ${patch_files[@]}|tr  ' '  '\n'|sort -r|uniq -u|tr '\n' ' '))
            else
                local revert_patches=()
                for ((i=$((ret_code-2));i>=0;i--))
                do
                    if [[ ! -f ${patch_files[i]}.norevert ]]; then
                        revert_patches+=( ${patch_files[i]} )
                    fi
                done
            fi
            local dd=$(patch_dir_with_patches $dest_dir 1 0  ${revert_patches[@]})
            return $ret_code
        fi
    fi
    return 0
}

function g_dot_wifi_version() {
    if [[ -n $1 ]]; then
        echo ${1//_/.}
    else
        echo ${g_wifi_version//_/.}
    fi
}
function g_dash_wifi_version() {
    if [[ -n $1 ]]; then
        echo ${1//./_}
    else
        echo ${g_wifi_version//./_}
    fi
}

function _mark_patch_versioned_done_wifi() {
    _dest_dir=$1
    [[ $g_patch_version_file_update -eq 1 ||  ! -d $_dest_dir ]] && return 0
    _wifi_version_file="$g_top_dir/patch.list.bspwifi"
    if [[ -f $_wifi_version_file ]]; then
        _all_rb_patches=($(awk -v wifi_ver="^$(g_dash_wifi_version).*" '{if (match($0,wifi_ver)) {print $2}}' $_wifi_version_file))
        for _rb_patch in  ${_all_rb_patches[@]}; do
            _rb_num=$(echo $_rb_patch|sed -Ee 's/.*(rb[0-9]+).*/\1/g')
            _patchfile=$(find ${_dest_dir}/ -maxdepth 1 -regex "^.*${_rb_num}.*patch$")
            if [[ -n $_patchfile ]]; then
                touch ${_patchfile}.done
            fi
        done
    fi
}

function _mark_patch_versioned_done() {
    [[ $g_patch_version_file_update -eq 1 ]] && return 0
    local patch_version_repo=$1
    local patch_src_dir=$2
    local ppfile
    if [[ -f $patch_version_repo && -d $patch_src_dir ]]; then
       local existing_patches=($(awk '{ if (match($0,"^- {csp: CS[0-9]+.*")) {match($0,"SWBCACPE-[0-9]+");print substr($0,RSTART, RLENGTH)}}' $patch_version_repo))
       if [[ -n $existing_patches ]]; then
            existing_patches=(${existing_patches[@]//ref:/})
            existing_patches=($(echo ${existing_patches[@]}|tr  ' '  '\n'|sort -n|uniq |tr '\n' ' '))
            for ppfile in ${existing_patches[@]}
            do
                _patchfiles=($(find $patch_src_dir/ -name *$ppfile*.patch))
                for _patchfile in ${_patchfiles[@]}; do
                    if [[ -n $_patchfile && ! -f $_patchfile.rdone ]]; then
                        touch $_patchfile.done
                    fi
                done
            done
        fi
    fi
}

# Main Function to handle patches
# Use global variables to retrive patches from right folder
# Parameters:
#    in: type of patch
#   out: (maxpatch num, patch resultstring)

function  wl_handle_patch() {

    [[ $g_brcm_patching_status -ne 0  && $g_internal_build -eq 0 ]] && return 1
    local is_test=0
    [[  $g_brcm_patching_dryrun -eq 1 ]] && is_test=1
    local patch_src_dir="$g_patch_src_root/patches/$g_bsp_rel_version"
    local patch_dst_dir=$g_top_dir
    local new_patch_num_name="g_new_patch_${1}_num";
    local new_patch_num_missing_name="g_new_patch_${1}_missing_num";
    _dpp "$LINENO:wl_handle_patch: $1"
    [[ $g_patch_version_file_update -ne 1 ]] && eval "${new_patch_num_name}=0"
    case  $1 in

        BSP)
            patch_src_dir=$patch_src_dir/bsp
            _mark_patch_versioned_done  $g_top_dir/patch.version $patch_src_dir
            _mark_patch_versioned_done  $g_top_dir/patch.list.wifi  $patch_src_dir
            ;;

        BSPWIFI)
            patch_src_dir="$patch_src_dir/wifi/bspwifi"
            _mark_patch_versioned_done  $g_top_dir/patch.version $patch_src_dir
            _mark_patch_versioned_done  $g_top_dir/patch.list.bspwifi  $patch_src_dir
            ;;

        RDKBSYS)
            patch_src_dir=$patch_src_dir/rdkb/system
            patch_dst_dir="$(abspath $g_top_dir/../)/meta-rdk-broadcom-bca-system"
            _mark_patch_versioned_done $patch_dst_dir/patch.list.wifi  $patch_src_dir
            _mark_patch_versioned_done $patch_dst_dir/patch.version  $patch_src_dir
            ;;

        RDKBSOC)
            patch_src_dir=$patch_src_dir/rdkb/soc
            patch_dst_dir="$(abspath $g_top_dir/../)/meta-rdk-broadcom-bca-soc"
            _mark_patch_versioned_done $patch_dst_dir/patch.version  $patch_src_dir
            _mark_patch_versioned_done $patch_dst_dir/patch.list.wifi  $patch_src_dir
            ;;

        WIFI)
            local missing_lable="Wm"
            patch_src_dir="$g_wifi_patch_src_dir/$g_wifi_version"
            patch_dst_dir=$g_wifi_impl_dir
            _mark_patch_versioned_done_wifi $patch_src_dir

            ;;
        *)
            ;;
    esac


    if [[ -d $patch_src_dir ]]; then
        if [[ $g_patch_version_file_update -eq 1 ]]; then
            #do after patch version file update
            if [[ $1 =~ BSP$ || $1 =~ ^RDKB(SOC|SYS)$ ]]; then
                pp_patch_version_modify $patch_src_dir/patch.list $patch_dst_dir/patch.list.wifi "BSP" $patch_src_dir
            elif [[ $1 =~ ^BSPWIFI$ ]]; then
                pp_patch_version_modify $patch_src_dir/patch.list $patch_dst_dir/patch.list.bspwifi "BSPWIFI" $patch_src_dir
            elif  [[ $1 =~ ^WIFI$ ]]; then
                pp_patch_version_modify_wifi $patch_src_dir/readme.txt $g_top_dir/patch.list.bspwifi $patch_src_dir
            fi
        else
            #do patch process
            patch_dir_from_dir $patch_dst_dir $patch_src_dir 0 $is_test $new_patch_num_name $new_patch_num_missing_name
            find $patch_src_dir/ -name *.done |xargs rm -f
        fi
    fi

}


# for wifi patches, a few releases may share the same patches, so we
# have  wifi_patch_mateher.ini file to record this

# parameters:
# in: wifi_version
#
function adjust_wifi_patch_version() {
    [[ ! -f $g_patch_src_root/patches/patch_mapping.ini ]] && return
    _wifi_version_=$1
    _wifi_version_prev=$1
    if [[ $# -gt 2 ]]; then
        g_wifi_patch_src_dir=$1/$3
        _wifi_patch_src_dir=$g_wifi_patch_src_dir
        _wifi_version_=$2
        _wifi_version_prev=$2
    else
         _wifi_patch_src_dir="$g_patch_src_root/patches/$g_bsp_rel_version/wifi/release"
         [[ $g_internal_build -eq 1 ]] && \
            _wifi_patch_src_dir="$g_patch_src_root/patches/$g_bsp_rel_version/wifi/internal"
    fi
    while [[ -n $_wifi_version_ && ! -d $_wifi_patch_src_dir/$_wifi_version_ ]]
    do
        local match_line=$(grep "^$_wifi_version_:[0-9_]\+$" $g_patch_src_root/patches/patch_mapping.ini)
        if  [[ -n $match_line ]]; then
            _wifi_version_=$(echo $match_line|sed "s/${_wifi_version_}:\([0-9_]\+\)/\1/")
        else
            _wifi_version_=""
        fi
    done
    if [[ -n $_wifi_version_ &&  $_wifi_version_ != $_wifi_version_prev ]]; then
        ln -sf $_wifi_patch_src_dir/../release/$_wifi_version_  $_wifi_patch_src_dir/../release/$_wifi_version_prev
        [[ -d  $_wifi_patch_src_dir/../internal/$_wifi_version_ ]] &&
        ln -sf $_wifi_patch_src_dir/../internal/$_wifi_version_  $_wifi_patch_src_dir/../internal/$_wifi_version_prev
    fi
    if [[ $# -gt 2 ]]; then
        eval "$4=$_wifi_version_"
    fi
}

#Init all needed global variables


#unset all global variables

function unset_global_variable() {
    unset g_top_dir
    unset g_router_dir
    unset g_wifi_patch_src_dir
    unset g_wl_dir
    unset g_bsp_rel_version
    unset g_wifi_versions
    unset g_wifi_impl
    unset g_wifi_impl_dir
    unset g_wifi_version
    unset g_wifi_patch_src
    unset g_new_patch
    unset g_brcm_patching_status
    unset g_brcm_patching_dryrun
    unset g_brcm_list_patches_only
    unset g_internal_build
    unset g_pp_wlimpl_dir
    unset g_patches_filelist
    unset g_patch_version_file_update
    unset g_has_new_patches
    unset g_show_patch_version_file
    unset g_vf_index
}

#input wifi_release_version:17_10_188_75 ##
function has_wifi_source() {
    local _input_version=$1
    local _has_source=0
    local wldir="$g_top_dir/bcmdrivers/broadcom/net/wl"
    local allwlimpls=($(find $wldir/ -maxdepth 2 -name "main"))
    for wlimpl in ${allwlimpls[@]}
    do
        [[ -z $(find $wlimpl/ -name "wlc.c") ]] && continue;
        wlimpl_get_ver $(dirname $wlimpl)  wlver
        [[ $_input_version == $wlver ]]  && _has_source=1 && break
    done
    echo $_has_source
}

function choose_wlimpl_options() {
    _patch_version=$2
    local wldir="$g_top_dir/bcmdrivers/broadcom/net/wl"
    local local_wlvers=()
    local allwlimpl
    if [[ -z $_patch_version && $g_internal_build -eq 1 ]]; then
        allwlimpl+=($(find $wldir/ -maxdepth 2 -name "main"))
    else
        local allwlimpls=($(find $wldir/ -maxdepth 2 -name "main"))
        for wlimpl in ${allwlimpls[@]}
        do
            [[ $g_internal_build -eq 0 && -z $(find $wlimpl/ -name "wlc.c") ]] && continue;
            wlimpl_get_ver $(dirname $wlimpl) wlver
            local_wlvers+=($wlver)
            if [[ $_patch_version == $wlver ]]; then
                if [[ (! -d $g_pp_wlimpl_dir) || $(abspath $g_pp_wlimpl_dir) == $(dirname $(abspath $wlimpl)) ]]; then
                     allwlimpl+=($wlimpl)
                fi
            fi
        done
    fi

    if [[ ${#allwlimpl[@]} -gt 1 ]]; then
        local sn=$(seq 1 1 ${#allwlimpl[@]})
        for i in ${sn[@]}
        do
            echo "$i: $(dirname ${allwlimpl[$((i-1))]})"
        done
        printf "Select one wlarch to continue:"
        read select
        if [[ ! -z $select  &&  $select =~ [0-9]+$ && $select -le ${#allwlimpl[@]}  && $select -gt 0 ]]; then
            select=$((select-1))
                eval "${1}=${allwlimpl[$select]}"
                g_has_wifi=1
            else
                echo "Not valid input[$select], quit!"
        fi
    elif [[ ! -z $allwlimpl ]]; then
        eval "${1}=$(dirname $allwlimpl)"
        g_has_wifi=1
    else
        if [[ $g_internal_build -eq 0 ]]; then
            _dpp "\nNo wifi release source of version $_patch_version to patch"
            if [[ -n ${local_wlvers[@]} ]]; then
                _dpp "You source tree wifi_version is: ${local_wlvers[@]}"
            fi
        fi
    fi
}

#get version number array from version file
function _get_version() {
    local version_file=$1
    local class=$2
    if [[ -n $3 ]]; then
        local version=($(awk -F "$3" '{if (match($1,"^'${class}'")) {print $2}}'  $version_file))
    else
        local version=($(awk '{if (match($1,"^'${class}'")) {print $2}}'  $version_file))
    fi
    if [[ -n $version && ! $version =~ None ]]; then
        local output=($version)
        local versions=$(echo $version|sed 's/\w*v\([0-9c]\+\)[B|W|S]\([0-9c]\+\)/\1 \2/')
        for ver in ${versions[@]};
        do
            local vers=(${ver//"c"/ })
            output+=(${vers[@]})
            [[ ${#vers[@]} -eq 1 ]] && output+=(0)
        done
        output+=(${wifi_version})
        echo ${output[@]}
    else
        echo "None 0 0 0 0"
    fi
}

#get expected and real from the same version file
function get_versions() {
    local version_file=$1
    local class=$2
    local output=($(_get_version $version_file $class ':'))
    output+=($(_get_version $version_file $class))
    if [[ ${output[0]} == ${output[5]} ]]; then
        output=('Match' ${output[@]})
    else
        local expected=(${output[@]:1:4})
        local patched=(${output[@]:6:4})
        local lable="Valid"
        for i in 0 1 2 3
        do

            if [[ ${expected[$i]} -gt ${patched[$i]} ]]; then
                lable="Invalid"
                break
            fi
        done
        output=($lable ${output[@]})
    fi
    echo ${output[@]}
}


#return "WIFI-17_10_251_32 WIFI 17_10_251_32"
function get_wifi_class() {
    local version_file=$1
    local version=($(awk -F ":" '{if (match($1,"^WIFI")) {print $1}}'  $version_file))
    for ver in  ${version[@]}
    do
        local _version=( $ver ${ver//-/ })
        if [[ -n $2 ]]; then
            echo ${_version[$2]}
        else
            echo ${_version[@]}
        fi
    done
}


function patch_version_strs() {
    _version_file=$1
    _simplified=$2
    _is_real_ver=$3
    _bsp_versions=($(get_versions $_version_file "BSP"))
    _rdk_versions=($(get_versions $_version_file "RDK"))
    _wifi_classes=($(get_wifi_class $_version_file))
    _version_strs=()
    _ver_pos=1
    [[ $_is_real_ver -eq 1 ]] && _ver_pos=6

    if [[ -z $_wifi_classes ]]; then
        [[ ${_bsp_versions[$_ver_pos]} != "None" ]] && _verstr="${_bsp_versions[$_ver_pos]}"
        [[ ${_rdk_versions[$_ver_pos]} != "None" ]] && _verstr+="-${_rdk_versions[$_ver_pos]}"
        _verstr=$(simple_version $_verstr)
        _version_strs+=(${_verstr//_/.})
    else

        for i in $(seq 0 $((${#_wifi_classes[@]}/3-1)))
        do

            local _wifi_class=${_wifi_classes[$((i*3))]}
            [[ -n $g_wifi_version  && ! $_wifi_class =~ .*$g_wifi_version ]] && continue
            local wifi_versions=($(get_versions $_version_file $_wifi_class))
            _verstr=""
            [[ ${_bsp_versions[$_ver_pos]} != "None" ]] && _verstr="${_bsp_versions[$_ver_pos]}"
            [[ ${wifi_versions[$_ver_pos]} != "None" ]] && _verstr+="-${wifi_versions[$_ver_pos]}"
            [[ ${_rdk_versions[$_ver_pos]} != "None" ]] && _verstr+="-${_rdk_versions[$_ver_pos]}"
            _verstr=$(simple_version $_verstr)
            if [[ -z $_simplified || $_simplified -eq 0 ]]; then
                _verstr+="-${_wifi_classes[$((i*3+2))]}.cpe${g_bsp_rel_version}"
            fi
            _version_strs+=(${_verstr//_/.})
        done
    fi
    echo ${_version_strs[@]}


}

function clean_patch_source() {
    if [[ $g_internal_build -ne 1 ]]; then
        # clean patches under /tmp for release patching when the patching is successful
        [[ $g_patch_src_root != $g_top_dir ]] && rm -rf $g_patch_src_root
        if [[ $g_has_new_patches -gt 0 ]]; then
            _new_patches="$(get_patch_package $g_new_patch_version)"
            #add patching history
            _has_history=$(awk '/##### PATCHING HISTORY.*/{print $0}' $g_current_patch_version)
            [[ -z $_has_history ]] && echo -e "\n##### PATCHING HISTORY #####" >>$g_current_patch_version
            _patch_history="Patched:${_new_patches/_} on $(date '+%Y-%m-%d:%H:%M:%S')"
            sed -i "/^##### PATCHING HISTORY.*/a $_patch_history" $g_current_patch_version
        fi
    fi
    rm -f $g_patches_filelist
    [[ $g_has_new_patches -gt 0 && $g_patch_incomplete  -ne 1  ]] && rm -f $g_new_patch_version
    return 0
}


#read patch packages from brcm_patch_version where patching history is kept
function get_patch_package() {
    _packages=($(awk '/^Patched:.*/{printf("%s %s ", $1,$2);}' $1))
    for i in $(seq 0 $((${#_packages[@]}/2-1))); do
         _time=${_packages[$((i*2+1))]}
         _patch_file=${_packages[$((i*2))]}
         _patch_file=${_patch_file/Patched:/_}
         echo "$_patch_file $_time"
    done
}


function get_patches_ready() {
    _dpp $LINENO ${FUNCNAME[@]}
    if [[ $g_has_new_patches -gt 0 ]]; then
        # if there is new relase patch , we use external directories for temp patch
        _patches=($(get_patch_package $g_new_patch_version))
        _dpp $LINENO ${FUNCNAME[0]} "patch package:$_patches"
        if [[ ! $_patches =~ .*$g_bsp_rel_version.* ]]; then
            warning "\nPackage:$_patches is not for BSP:$g_bsp_rel_version!"
            return 1
        fi
        _prj_dir=($(echo "${g_top_dir}${_patches[0]}${_patches[1]}$(whoami)"|md5sum))
        if [[ -f $g_top_dir/patches/pp_package_repo/$_patches ]]; then
            g_patch_src_root="/tmp/${_prj_dir[0]}"
            g_patch_package_filename=$(echo $_patches|sed 's/^_//g')
            rm -rf $g_patch_src_root
            mkdir -p $g_patch_src_root
            _dpp $LINENO ${FUNCNAME[0]} "g_patch_src_root:$g_patch_src_root"
            tar xvfz $g_top_dir/patches/pp_package_repo/$_patches -C $g_patch_src_root 2>&1 >/dev/null
            g_patches_filelist=$g_patch_src_root/.applied_patch_file_list

            print_file_list    "Added[p]"     "patches/pp_package_repo/$_patches"
            print_file_list    "Modified[p]" "patches/utility.sh"
            print_file_list    "Modified[p]" "dopatch"
            print_file_list    "Modified[p]" "patch.list.bspwif"
            print_file_list    "Modified[p]" "patch.list.wifi"
            print_file_list    "Modified[p]" "brcm_patch_version"
        fi
    fi
    g_patches_filelist=$g_patch_src_root/.applied_patch_file_list

}

#check new patch package with patched source to see if there is new patches
function check_patches() {
    _new_patch=$1
    _existing_patch=$2
    _pp_wlimpl_dir=$3
    _heath_check=0

    _dpp $LINENO "_new_patch:$_new_patch, _existing_patch:$_existing_patch"
    [[ ! -f $_new_patch  || ! -f $_existing_patch ]] && return 0
    # list/check are pretty much the same as one lists files, the other does not.
    # use dry_run to differentiate, run patch process to find out if there
    # is new patches available before anything
    g_brcm_patching_dryrun=0
    [[ $g_brcm_list_patches_only -eq 1 ]] && g_brcm_patching_dryrun=1
    g_brcm_list_patches_only=1
    pp_main $g_top_dir $_pp_wlimpl_dir

    [[ $_new_patch == $_existing_patch ]] && _heath_check=1
    _wifi_classes=($(get_wifi_class $_new_patch))

    _types=("BSP")
    for i in $(seq 0 $((${#_wifi_classes[@]}/3-1)))
    do
        [[ $(has_wifi_source ${_wifi_classes[$((i*3+2))]}) -eq 1 ]] && _types+=(${_wifi_classes[$((i*3))]})
    done
    [[ $g_has_rdkb_src -gt 0 ]] && _types+=("RDK")

    local _g_has_new_version=0
    local _g_has_old_version=0
    if [[ $_heath_check -eq 1 ]];then
        success "\n============ Current Patches Status ==================="
        printf "%-19s %-3s %-12s %-2s %-2s %-2s %-9s\n" "Component"  "|" "Expected"  "|" "?" "|" "Existing"
    else
        success "\n============ New Patches available ===================="
        printf "%-19s %-3s %-12s %-2s %-2s %-2s %-9s\n" "Component"  "|" "New"  "|" "?" "|" "Existing"
    fi
    success "-------------------------------------------------------"
    for _class in ${_types[@]}
    do
        _new_versions=($(get_versions $_new_patch $_class))
        _exsiting_versions=($(get_versions $_existing_patch $_class))
        _new_version=(${_new_versions[@]:2:5})
        _exsiting_version=(${_exsiting_versions[@]:7:4})
        _has_new_version=0
        _has_old_version=0
        _has_equal_version=0
        _compare_result=""
        for i in 0 1 2 3
        do
            if [[ ${_new_version[$i]} -gt ${_exsiting_version[$i]} ]]; then
                _has_new_version=1
                _g_has_new_version=1
            elif [[ ${_new_version[$i]} -lt ${_exsiting_version[$i]} ]]; then
                _has_old_version=1
                _g_has_old_version=1
            else
                _has_equal_version=1
            fi
        done
        [[ $_has_old_version -gt 0 ]] && _compare_result+="<"
        [[ $_has_equal_version -gt 0 ]] && _compare_result+="="
        [[ $_has_new_version -gt 0 ]] && _compare_result+=">"
        printf "%-19s %-3s %-12s %-2s %-2s %-2s %-9s\n" ${_class} "|" ${_new_versions[1]}\
            "|" $_compare_result  "|" ${_exsiting_versions[6]}
    done
    success "=======================================================\n"

    if [[ $_heath_check -eq 0 ]]; then
        if [[ $g_new_patch -gt 0  || -f $g_new_patch_version ]]; then
            normaln "Run " && boldn "#./dopatch apply " && \
            normal 1 "to apply" && normal 2
        else
            success "\nAll patches in new patch package were patched.No action needed!\n"
        fi
    else
        _versions=($(patch_version_strs $_existing_patch))
        if [[ $_g_has_old_version -eq 0 && $_g_has_new_version -eq 0 ]]; then
            success "================= Current Patch versions: ============="
            for _version in ${_versions[@]}
            do
                bold "$_version"
            done
            success "=======================================================\n"
        elif [[ $_g_has_old_version -eq 1 ]]; then
            local _real_versions=($(patch_version_strs $_existing_patch 1 1))
            local i=0
            _versions_simple=($(patch_version_strs $_existing_patch 1))
            alert "======== Source was patched with partial patch. ======="
            for _version in ${_versions_simple[@]}
            do
                printf "%-25s:  %-20s\n" "Last Applied Patch" "$(last_patched)"
                printf "%-25s:  %-20s\n" "Current Patched at" ${_real_versions[$i]}
                i=$((i+1))
            done
            alert "=======================================================\n"
        else
            warning "======================================================="
            alert "Suggest re-apply patch package:" && bold "${_versions[0]}"
            alert "before applying any new patch."
            warning "======================================================="
        fi
    fi
}

#after successful patch, update current patch version file with new patch number
function sync_version() {
    #if it is release patch, then sync expected version
    if [[ -f $g_new_patch_version ]]; then
        _wifi_class=($(get_wifi_class $g_new_patch_version))
        _types=("BSP" ${_wifi_class[@]} "RDK")
        _has_new_version=0
        for class in ${_types[@]}; do
            _new_version=($(get_versions $g_new_patch_version $class))
            #if new version include no patch, no need to update anything
            [[ $((${_new_version[2]}+${_new_version[4]})) -eq 0 ]] && continue
            _current_patch_version=($(get_versions $g_current_patch_version $class))
            if [[ ${_current_patch_version[1]} == "None" ]]; then
                _has_new_version=1
            else
                for i in 2 3 4 5; do
                    if [[ ${_new_version[$i]} -gt ${_current_patch_version[$i]} ]]; then
                        _has_new_version=1
                        _current_patch_version[$i]=${_new_version[$i]}
                    fi
                done
            fi
            _new_ver=$class
            [[ $class =~ ^WIFI ]] && local _new_ver="WIFI"
            if [[ $_has_new_version -eq 1  && ${_new_version[1]} != "None" ]]; then
                _new_ver+="v${_new_version[2]}"
                [[ ${_new_version[3]} -ne 0 ]] && _new_ver+="c${_new_version[3]}"
                [[ $_new_ver =~ ^BSP.* ]] && _new_ver+="W"
                [[ $_new_ver =~ ^WIFI.* ]] && _new_ver+="B"
                [[ $_new_ver =~ ^RDK.* ]] && _new_ver+="S"
                _new_ver+="${_new_version[4]}"
                [[ ${_new_version[5]} -ne 0 ]] && _new_ver+="c${_new_version[5]}"
                if [[ ${_current_patch_version[1]} != "None" ]]; then
                    sed -i "s/^${class}:${_current_patch_version[1]}\(.*\)/$class:$_new_ver\1/g" $g_current_patch_version
                else
                    sed -i "/^##### PATCH VERSIONS.*/a $class:$_new_ver" $g_current_patch_version
                fi
            fi
        done
    fi

    update_patched_version "BSP"
    [[ $g_has_rdkb_src -gt 0 ]] && update_patched_version "RDK"
    [[ $g_has_wifi -gt 0  && -n $g_wifi_impl_dir ]] && update_patched_version "WIFI"
    return 0
}

# new patch version will be always equal or bigger than expected version when
# module patch are applied. new patches will always add onto existing one and
# use the max number.
function new_patch_verison() {
    _class=$1
    case $_class in
        BSP)
            _value1=$g_new_patch_BSP_num
            _value2=$g_new_patch_BSPWIFI_num
            _format="BSPv%dW%d"
            ;;
        WIFI)
            _value1=$g_new_patch_WIFI_num
            _value2=$g_new_patch_BSPWIFI_num
            _class="WIFI-$(g_dash_wifi_version)"
            _format="WIFIv%dB%d"
            ;;
        RDK)
            _value1=$g_new_patch_RDKBSOC_num
            _value2=$g_new_patch_RDKBSYS_num
            _format="RDKv%dS%d"
            ;;
    esac
    _new_version=($(get_versions $g_current_patch_version $_class))
    _value1=$((${_new_version[7]}+$_value1))
    _value2=$((${_new_version[9]}+$_value2))

    [[ $_value1 -lt ${_new_version[2]} ]] && _value1=${_new_version[2]}
    [[ $_value2 -lt ${_new_version[4]} ]] && _value2=${_new_version[4]}

    case $_class in
        BSP)
            [[ -n $g_new_patch_BSP_missing_num ]] &&
                _value1=$((_value1-$g_new_patch_BSP_missing_num))
            [[ -n $g_new_patch_BSPWIFI_missing_num ]] &&
            _value2=$((_value2-$g_new_patch_BSPWIFI_missing_num))
            ;;
        WIFI)
            [[ -n $g_new_patch_WIFI_missing_num ]] &&
                _value1=$((_value1-$g_new_patch_WIFI_missing_num))
            [[ -n $g_new_patch_BSPWIFI_missing_num ]] &&
                _value2=$((_value2-$g_new_patch_BSPWIFI_missing_num))
            ;;
        RDK)
            [[ -n $g_new_patch_RDKBSOC_missing_num ]] && 
                _value1=$((_value1-$g_new_patch_RDKBSOC_missing_num))
            [[ -n $g_new_patch_RDKBSYS_missing_num ]] &&
                _value2=$((_value2-$g_new_patch_RDKBSYS_missing_num))
            ;;
    esac
    _value="printf \"$_format\" $_value1 $_value2"
    _value=$(eval $_value)
    eval "$2=($_value $(($_value1+$_value2)))"
}


function update_patched_version() {
    _class=$1
    new_patch_verison $_class _class_value
    if [[ ${_class_value[1]} -gt 0 ]]; then
        _current_version=($(get_versions $g_current_patch_version $_class))
        _new_version=($(get_versions $g_new_patch_version $_class))
        if [[ ${_current_version[1]} == "None" ]]; then
            sed -i "/^##### PATCH VERSIONS.*/a $_class:$_class_value $_class_value" $g_current_patch_version
        else
            sed -i "s/\(^${_class}:\w\+\).*/${_class}:${_new_version[1]} $_class_value/g" $g_current_patch_version
        fi
    fi
    return 0
}

function show_patchfile_list() {
    if [[ -f $g_patches_filelist ]]; then
        success "\n=========== Impacted Files (should in Version Control [p] means possilbe)============"
        sed -i "s|${g_top_dir%%\/}/||g" $g_patches_filelist
        _files="$(sort $g_patches_filelist|uniq)"
        echo "$_files"
        success "======================================================================================\n"
    fi
}

function apply_patch() {
    ## first to dry-run patches.only do patch if no failure
   _dpp $LINENO "AP:start patch dryrun ...."
    g_brcm_patching_dryrun=1 && pp_main $1 $2
    ## for internal build, ignore patch failure and keep patching.
    if [[ $g_brcm_patching_status -eq 0  || $g_internal_build -eq 1 ]]; then

            [[ $g_brcm_patching_status -gt 0  || $g_internal_build -eq 1 ]]  && alert "Attention-Internal Patch has failure."
            g_brcm_patching_dryrun=0
            _dpp $LINENO "AP:start patching ...."
            [[ $g_new_patch -gt 0 ]] && pp_main $1 $2 #apply patch
            _dpp $LINENO "AP:start update patch version ...."
            g_patch_version_file_update=1 && pp_main $1 $2 #update patch version files
            _dpp $LINENO "AP: create/update epibsppatch version...."
            if [[ $g_has_new_patches -gt 0 ]]; then
                sync_version
                create_epibsppatch_header
                show_patchfile_list
            fi
            clean_patch_source
            if [[ $g_new_patch -gt 0 ]]; then
                success "Patching $g_patch_package_filename Successful!\n"
            else
                echo -e "\n==============================================="
                success "All patches have been applied, no new patches."
                echo -e  "==============================================\n"
            fi
    fi
}

function pp_main() {
    wl_handle_patch "BSP"
    wl_handle_patch "BSPWIFI"
    if [[ $g_has_rdkb_src -gt 0 ]];  then
        wl_handle_patch "RDKBSOC"
        wl_handle_patch "RDKBSYS"
    fi

    [[ $g_has_wifi -eq 1  && -n $g_wifi_version ]] && wl_handle_patch "WIFI"

    if [[ -z $g_brcm_list_patches_only || $g_brcm_list_patches_only -eq  0 ]]; then
        if [[ $g_new_patch -gt 0 ]]; then
            if [[ $g_brcm_patching_status -gt 0 ]]; then
                alert "Patching $g_patch_package_filename has failure,Please check\n"
            fi
        fi
    fi
}

function adjust_bsp_patch_version() {
    local _bsp_patch_dir="$g_patch_src_root/patches"
    [[ ! -f $_bsp_patch_dir/patch_mapping.ini ]] && return
    local _bsp_version_org=$1
    local match_items=($1)
    if [[ -z $(grep "^-$_bsp_version_org$" $_bsp_patch_dir/patch_mapping.ini) ]]; then
        match_items+=($(echo $1|sed -n 's/\([1-9]\.[0-9]\+L\.[0-9]\+.[1-9]\).*/\1/p'))
        match_items+=($(echo $1|sed -n 's/\([1-9]\.[0-9]\+L\.[0-9]\+\).*/\1/p'))
    fi
    for _bsp_version_ in ${match_items[@]}
    do
        [[ -n $_bsp_version_ &&  -d $_bsp_patch_dir/$_bsp_version_  ]] && break
        while [[ -n $_bsp_version_ && ! -d $_bsp_patch_dir/$_bsp_version_  ]]
        do
            local match_line=$(grep "^$_bsp_version_:.\+$" $_bsp_patch_dir/patch_mapping.ini)
            if  [[ -n $match_line ]]; then
                _bsp_version_=$(echo $match_line|sed "s/${_bsp_version_}:\(.\+\)/\1/")
            else
                _bsp_version_=""
            fi
        done
    done
    if [[ -n  $_bsp_version_ && $_bsp_version_ != $_bsp_version_org  && ! -d $g_top_dir/patches/$_bsp_version_org ]]; then
           ln -sf $g_top_dir/patches/$_bsp_version_  $g_top_dir/patches/$_bsp_version_org
    fi
}

# input: (1)top_dir (2)wlimpl_dir (3)sourced #
function _base_global() {
    g_wl_dir="$g_top_dir/bcmdrivers/broadcom/net/wl"
    g_has_wifi=0;
    if [[ -f $g_top_dir/version.make ]]; then
        source $g_top_dir/version.make
        g_bsp_rel_version="$BRCM_VERSION"."$BRCM_RELEASE"L."$BRCM_EXTRAVERSION"
    else
        warning "why there is not version.make under $g_top_dir"
        return 0
    fi
    g_has_rdkb_src=0;
    [[ -d $g_top_dir/../meta-rdk-broadcom-bca-system ]] && g_has_rdkb_src=1

    g_internal_build=0;
    [[ -f $g_top_dir/patches/pputil.sh ]] && g_internal_build=1;
    g_patch_src_root=$g_top_dir
    adjust_bsp_patch_version $g_bsp_rel_version  g_bsp_rel_version
    g_new_patch=0
    g_new_patch_RDKBSOC_num=0
    g_new_patch_RDKBSYS_num=0
    g_new_patch_BSP_num=0
    g_new_patch_BSPWIFI_num=0
    g_new_patch_WIFI_num=0
    g_brcm_patching_status=0
    g_show_patch_version_file=0
    g_patch_incomplete=0
    g_vf_index=1
}

# input: (1)top_dir (2)wlimpl_dir (3)sourced #
function _patch_global() {
    g_new_patch=0
    g_new_patch_RDKBSOC_num=0
    g_new_patch_RDKBSYS_num=0
    g_new_patch_BSP_num=0
    g_new_patch_BSPWIFI_num=0
    g_new_patch_WIFI_num=0
    g_has_wifi=0
    g_patch_version_file_update=0;
    g_wifi_impl_dir=""

    [[ -d $2  && ! $2 =~ NONE ]] && g_wifi_impl_dir=$2 && g_has_wifi=1
    _wifi_classes=($(get_wifi_class $g_new_patch_version 2))
    for _wifi_class in ${_wifi_classes[@]}; do
        if [[ -n $_wifi_class ]]; then
            choose_wlimpl_options g_wifi_impl_dir $_wifi_class #set g_has_wifi as well
        fi
        [[ $g_has_wifi -gt 0 ]] && break
    done
    _dpp $LINENO .......g_has_wifi:$g_has_wifi
    if [[ $g_has_wifi -ne 0 && -d $g_wifi_impl_dir ]]; then
        wlimpl_get_ver $g_wifi_impl_dir  wlver
        adjust_wifi_patch_version  $wlver wlver
    fi

    if [[ $g_has_wifi -eq 1 ]]; then
        g_wifi_impl=$(echo $g_wifi_impl_dir|sed -n 's/.*\(impl[0-9]\+[^/]*\).*/\1/p')
        g_wifi_impl_dir=$g_wl_dir/${g_wifi_impl}
        wlimpl_get_ver $g_wifi_impl_dir  g_wifi_versions >/dev/null
        g_wifi_version=${g_wifi_versions[0]}
        g_wifi_patch_src=${g_wifi_versions[1]} # release or internal
        g_bsp_src_last=0
        g_wifi_src_last=0
    fi
    get_patches_ready
    [[ $g_has_wifi -eq 1 ]] && g_wifi_patch_src_dir=$g_patch_src_root/patches/$g_bsp_rel_version/wifi/$g_wifi_patch_src
    # for internal build with release patch package to build with internal wifi, use internal source
    if [[ $g_internal_build -eq 1 && $g_wifi_patch_src == "internal" && ! -d $g_wifi_patch_src_dir ]]; then
        g_wifi_patch_src_dir=$g_top_dir/patches/$g_bsp_rel_version/wifi/$g_wifi_patch_src
    fi
    _dpp $LINENO g_wifi_patch_src_dir:$g_wifi_patch_src_dir
    g_brcm_patching_status=0
}

(return 0 2>/dev/null) && sourced=1 || sourced=0
if [[ $sourced -eq 0 ]]; then
    g_top_dir=$(abspath $(dirname $0)/../)
    g_current_patch_version=$g_top_dir/brcm_patch_version
    [[ ! -f $g_current_patch_version ]] && create_version_file $g_current_patch_version
    g_has_new_patches=1
    [[ -d $1 ]] && g_top_dir=$(abspath $1)

    g_pp_wlimpl_dir="$(echo "$2"| sed -En 's/(.*impl[0-9]+).*/\1/p')"

    g_new_patch_versions=()
    _new_patch_versions=($(ls -rt $g_top_dir/.brcm_patch_version* 2>/dev/null))

    if [[ ! -z $g_pp_wlimpl_dir  && -d $g_pp_wlimpl_dir ]]; then
        if [[  -n $_new_patch_versions ]]; then
            _dpp $LINENO ....
            wlimpl_get_ver $g_pp_wlimpl_dir  wlver >/dev/null
            _dpp $LINENO ....
            for _new_patch_version in ${_new_patch_versions[@]}; do
                _wifi_classes=($(get_wifi_class $_new_patch_version 2))
                if [[ $_wifi_classes == $wlver ]]; then
                    g_new_patch_versions+=($_new_patch_version)
                fi
            done
        fi
    else
        g_new_patch_versions+=(${_new_patch_versions[@]})
        g_pp_wlimpl_dir="NONE"
    fi

    [[ -z $g_new_patch_versions ]] && g_new_patch_versions=($g_current_patch_version) && g_has_new_patches=0

    _base_global $g_top_dir $g_pp_wlimpl_dir $sourced

    if [[ ${#g_new_patch_versions[@]} -gt 1 ]]; then
        success "\n========================================"
        success "There are ${#g_new_patch_versions[@]} new patches existing"
        success "==========================================\n"
        g_show_patch_version_file=1
    fi

    for g_new_patch_version in ${g_new_patch_versions[@]}; do
    [[ $g_show_patch_version_file -gt 0 ]] && alert "PatchPacakge $g_vf_index:$(basename $g_new_patch_version)" && g_vf_index=$((g_vf_index+1))
    [[ ! $1 =~ list ]] && unset g_brcm_list_patches_only
    _patch_global $g_top_dir $g_pp_wlimpl_dir $sourced
    case $1 in
        list|check)
            previous_patch_migration
            if [[ $g_has_new_patches -gt 0 ]]; then
                check_patches $g_new_patch_version $g_current_patch_version $g_wifi_impl_dir
            else
                [[ $g_internal_build -eq 1 && ! -f $g_current_patch_version ]] && create_version_file $g_current_patch_version
                check_patches $g_current_patch_version $g_current_patch_version $g_wifi_impl_dir
            fi
            ;;

        apply)
            previous_patch_migration
            apply_patch $g_top_dir $g_wifi_impl_dir
            ;;

        wlversion)
            [[ -z $2 || ! -d $2 ]] && warning "wlversion need wlimpl dir" && exit 0
            wlimpl_get_ver $2  g_wifi_versions
            if [[ ${#g_wifi_versions[@]} -eq 3 ]]; then echo ${g_wifi_versions[2]}; else echo ""; fi
            ;;

        snapshot)
            #snapshot patch status for reporting to
            _date_str=$(date '+%Y-%m-%d:%H:%M:%S')
            _snapshot_dir="/tmp/brcm_patch_snapsot_$_date_str"
            mkdir -p $_snapshot_dir
            ../dopatch check|sed 's/\x1B\[[0-9;]\{1,\}[A-Za-z]//g' >>$_snapshot_dir/current_patch_status.log
            [[ -f $g_current_patch_version ]] && cp $g_current_patch_version $_snapshot_dir/
            cp $g_top_dir/dopatch $_snapshot_dir/
            cp $g_top_dir/patches/utility.sh $_snapshot_dir/
            if [[ -d $g_top_dir/patches/pp_package_repo ]]; then
                mkdir -p $_snapshot_dir/patches/pp_package_repo/
                cp $g_top_dir/patches/pp_package_repo/*.tgz  $_snapshot_dir/patches/pp_package_repo/
            fi
            _snapshot_zip="$g_top_dir/brcm_${g_bsp_rel_version}_patch_snapshot.tar.gz"
            pushd $_snapshot_dir 2>&1 >/dev/null
            tar cvfz $_snapshot_zip  ./* 2>&1 >/dev/null
            rm -rf $_snapshot_dir
            success "\n====================================================="
            boldn "./$(basename $_snapshot_zip)" && normal 1 " is generated."
            echo -e "Please send this zip file to BRCM support for patching "
            echo -e "issue investigation."
            success "=====================================================\n"
            ;;
        *)
            if [[ $# -ge 2 && -d $1 && -d $2 ]]; then
                previous_patch_migration $2
                apply_patch $1 $2 $3
            else
                warning "Unsupported $@"
            fi
            ;;
    esac
    done
    unset_global_variable
else
    g_top_dir=$(abspath $(dirname ${BASH_SOURCE[0]})/../)
    _base_global $g_top_dir "NONE" $sourced
fi

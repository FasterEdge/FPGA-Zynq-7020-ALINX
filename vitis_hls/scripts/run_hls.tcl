# run_hls.tcl — Vitis HLS 脚本：OneKey（HMAC-SHA256）加速核
# 用法：
#   cd vitis_hls/scripts
#   vitis_hls -f run_hls.tcl
# 产物：solution1/impl/ip 下的 Vivado IP（fe_onekey_issue / fe_onekey_verify）
set script_dir [file dirname [file normalize [info script]]]
set repo_dir   [file normalize [file join $script_dir ..]]

open_project -reset fe_onekey_proj
set_top fe_onekey_issue
add_files [file join $repo_dir src fe_hmac_sha256.cpp]
add_files [file join $repo_dir src fe_onekey_hls.cpp]
add_files -tb [file join $repo_dir tb fe_onekey_tb.cpp]
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default

# 主要优化目标：SHA-256 压缩循环流水化（流水指令已写在
# src/fe_hmac_sha256.cpp 的 sha256_compress/t 循环内）

csim_design
csynth_design
cosim_design -trace_level none
export_design -format ip_catalog

puts "INFO: HLS done, IP at [file join $repo_dir scripts fe_onekey_proj solution1 impl ip]"
exit

make clean
make 
echo 0 >/sys/devices/system/cpu/cpu1/online 
echo 0 >/sys/devices/system/cpu/cpu2/online 
echo 0 >/sys/devices/system/cpu/cpu3/online
#echo 0 >/sys/devices/system/cpu/cpu4/online
#echo 0 >/sys/devices/system/cpu/cpu5/online
#echo 0 >/sys/devices/system/cpu/cpu6/online
#echo 0 >/sys/devices/system/cpu/cpu7/online 
date 
insmod mem_org_tool.ko 
date 
echo 1 >/sys/devices/system/cpu/cpu1/online
echo 1 >/sys/devices/system/cpu/cpu2/online
echo 1 >/sys/devices/system/cpu/cpu3/online
#echo 1 >/sys/devices/system/cpu/cpu4/online
#echo 1 >/sys/devices/system/cpu/cpu5/online
#echo 1 >/sys/devices/system/cpu/cpu6/online
#echo 1 >/sys/devices/system/cpu/cpu7/online
rmmod mem_org_tool 

#!/system/bin/sh
count=0;
echo "number of fds  : pid  :  command"

while true; do
    if [ $(($count%2)) == 0 ]; then 
        for x in `ps -eF| awk '{ print $2 }'`; do
            echo `ls /proc/$x/fd 2> /dev/null | wc -l` $x `cat /proc/$x/cmdline 2> /dev/null`; 
        done | sort -n -r | head -n 20
    else
        lsof
    fi 

    echo ""
    let count++
    sleep `echo "20*60" | bc`
done

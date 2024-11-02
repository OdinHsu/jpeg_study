#! /bin/bash
disparray=[]
dispnum=0
intern=eDP-1 
defaut=xrandr
primarynum=0
otherscreen="no"

setxrandr()
{
    connectedOutputs=$(xrandr | awk '$2 == "connected"{print $1}')
    for display in $connectedOutputs
    do
    if [ ${display:0:3} == "DVI" ];then 
        disparray[$dispnum]=$display
        dispnum=$(expr $dispnum + 1)
    elif [ $primarynum -eq 0 ];then
        primarynum=$(expr $primarynum + 1)
        intern=$display
    else
        otherscreen=$display
    fi
    done
}

setdisplay()
{
    if [ $otherscreen == "no" ];then 
        $defaut --output $intern --primary --auto --output $disparray --right-of $intern --auto > /dev/null 2> /opt/Racer_log.log
    else    
        $defaut --output $intern --primary --auto --output $disparray --right-of $otherscreen --auto > /dev/null 2> /opt/Racer_log.log
    fi
}

setxrandr
setdisplay
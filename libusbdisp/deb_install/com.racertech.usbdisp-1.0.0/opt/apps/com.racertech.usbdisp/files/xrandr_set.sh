#! /bin/bash
display_number="$(w -hs | awk '{print $3}' | sort -u)"
xauthority_locate="$(printenv XAUTHORITY)"
os_name="$(lsb_release -is)"
os_name_NFSChina="${os_name:0:8}"

lisproder=$(xrandr --listproviders | awk '$11 == "outputs:" && $12 == "1" && $16 == "name:modesetting"{print $2}')
disparray=[]
dispnum=0
intern=eDP-1 
defaut=xrandr
primarynum=0
otherscreen="no"

export DISPLAY=$display_number
export XAUTHORITY=$xauthority_locate


echo $display_number 
echo $xauthority_locate 

setprovideroutputsource()
{   
    for p in $lisproder
    do
        xrandr --setprovideroutputsource ${p:0:1} 0
    done
    xrandr --auto
    xrandr
}

xrandrset()
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
    if [ ${#disparray[@]} != 0 ]; then 
        if [ $otherscreen == "no" ];then 
            $defaut --output $intern --primary --auto --output $disparray --right-of $intern --auto
        else    
            $defaut --output $intern --primary --auto --output $disparray --right-of $otherscreen --auto
        fi
    fi
}
do_something(){
exit 0
}
if [ "$os_name_NFSChina" == "NFSChina" ];
then 
echo "" 
setprovideroutputsource
xrandrset
else
do_something
fi

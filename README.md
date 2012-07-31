# Intro

xsetprop -- set window properties. I write it couse xprop can't set list of atoms.  

For example:
    xprop -f _NET_WM_STATE 32a -set _NET_WM_STATE _NET_WM_STATE_ABOVE
works fine
    xprop -f _NET_WM_STATE 32a -set _NET_WM_STATE _NET_WM_STATE_ABOVE,_NET_WM_SKIP_TASKBAR
don't work
    xsetprop --remap --mode='replace' --id=WIN_ID --format='32a' --propname='_NET_WM_STATE' --value='_NET_WM_STATE _NET_WM_STATE_ABOVE,_NET_WM_SKIP_TASKBAR'
OK. :)

# Compilation

    make

# Usage

    xsetprop --id=window_id --format=32a --propname=WM_ICON_NAME --value=test [--mode=[replace]|append|prepend] [--remap]

--id or -i : window id, you can get it from xwininfo.  
--format or -f : format of property value it's like in xprop. 32a for atoms, 8s for strings, etc. See man xprop.  
--propname or -p : name of property you want to set.  
--value or -v : new value of property(if --mode=replace or not specified) or value to append or prepend to property(if --mode=append or --mode=prepend).  
--mode or -m : replace for discard old value, append/prepend to append/prepend to old value.  
--remap : if this flag is specified window will be unmapped and then mapped again. It helps WMs to see changes of properties sometimes.  
Alternately you can use : xsetprop --id ID --atom ATOMNAME --value VALUE, so you don't need to specify --format and --propname. The same syntax for --string.

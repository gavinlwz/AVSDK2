#!/bin/sh
unset backup
backup=0
unset remove
remove=0
printf 'Do you want to back up the current folder? (y/n/q): '
    read yn
    case $yn in
      y | Y)
        backup=1
        ;;
      n | N)
        continue
        ;;
      q | Q)
        exit 1
        ;;
      *)
        echo ""
        echo "Please enter 'y', 'n', or 'q'."
        ;;
    esac
 
## backup
 
if [ backup ];then
 SUFFIX=_bak
 CWD=${PWD##*/}
 mkdir ../$CWD$SUFFIX
 cp -rf * ../$CWD$SUFFIX
 echo done!
fi
 
printf 'Do you want to clean the svn hidden folders? (y/n/q): '
    read yn
    case $yn in
      y | Y)
        remove=1
        ;;
      n | N)
        continue
        ;;
      q | Q)
        exit 1
        ;;
      *)
        echo ""
        echo "Please enter 'y', 'n', or 'q'."
        ;;
    esac
 
if [ remove ];then
 echo ""
 echo "recursively removing .svn folders from"
 pwd
 rm -rf `find . -type d -name .svn`
 rm -rf `find . -type d -name .git`
 echo ""
 echo done!
fi
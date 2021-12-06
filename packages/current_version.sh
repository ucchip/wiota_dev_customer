#!/bin/sh
# git version
git_version=$(git log | awk 'NR == 3' | cut -d"+" -f1 | cut -b 7- | awk '$1=$1')
old_git_version=$(cat $1  | grep GIT_UPDATE_VERSION_TIME | cut -d"\"" -f2)
echo "currnet git version time:"
echo $git_version
sed -i "s/$old_git_version/$git_version/g" $1
#git tag
git_crrent_tag=$(git tag | tail -1)
echo "current git tag:"$git_crrent_tag
old_git_tag=$(cat $1  | grep GIT_TAIL_TAG | cut -d"\"" -f2)
sed -i "s/$old_git_tag/$git_crrent_tag/g" $1
sync $1

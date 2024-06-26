#!/bin/sh
#
# Reject pushes that add commits from master branch to any stable
# release branch.
#
# --- Command line
refname="$1"
oldrev="$2"
newrev="$3"

if [ -z "$refname" -o -z "$oldrev" -o -z "$newrev" ]; then
    echo "Usage: $0 <ref> <oldrev> <newrev>" >&2
    exit 1
fi

if [ -z "${newrev##0*}" -o -z "${oldrev##0*}" ] ; then
    echo branch creation/deletion
elif echo "$refname" | grep -q '^refs/heads/V[0-9][0-9]*_[0-9][0-9]*[^0-9]' ; then
    let series=`echo "$refname" | sed 's|refs/heads/V[0-9]*_\([0-9]*\)[^0-9].*|\1|'`
    echo series is $series
    if (( (series / 2) == ((series + 1) / 2) )) ; then
        echo stable branch
        tmpfile=/tmp/git-commit-check.$$
        git rev-list $oldrev..refs/heads/master >$tmpfile
        git rev-list $oldrev..$newrev >>$tmpfile
        if sort $tmpfile | uniq -c | grep -q -v '^ *1 ' ; then
            badness=1
        fi
        rm -rf $tmpfile
        if [ -n "$badness" ] ; then
            echo "*** Rejecting push of development code to stable branch $refname" >&2
            exit 1
        else
            echo Commit is ok
        fi
    else
        echo dev branch
    fi
else
    echo Not a normal branch name
fi

exit 0

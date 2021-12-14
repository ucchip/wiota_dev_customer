#!user/bin/python
# _*_coding UTF-8 _*_

import git
import re
import os

def main():
    repo = git.Git(r'.\\')
    res = re.findall("Date:   (.+) \+", repo.log(-1))
    tags = re.findall("(.+)", repo.tag())
    path = r".\PS\app\include\uc_wiota_version.h"

    with open(path, 'r') as fp:
        content = fp.read()
        ver = re.findall("  \"(.+)\"", content)
        replceTag = content.replace(ver[0], tags[-1])
        replceTime = replceTag.replace(ver[1], res[0])
        fp = open(path, 'w')
        fp.write(replceTime)
    fp.close()

if __name__ == '__main__':
    main()

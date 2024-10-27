#! /bin/bash

git ls-files | grep -E "\.c$|\.h$" | grep -v -f <(git config --file .gitmodules --get-regexp path | awk '{ print $2 }') | xargs clang-format -i

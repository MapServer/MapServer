#!/bin/bash

git checkout branch-5-0
if $?; then exit; fi
git merge branch-4-10
if $?; then exit; fi
git checkout branch-5-2
if $?; then exit; fi
git merge branch-5-0
if $?; then exit; fi
git checkout branch-5-4
if $?; then exit; fi
git merge branch-5-2
if $?; then exit; fi
git checkout branch-5-6
if $?; then exit; fi
git merge branch-5-4
if $?; then exit; fi
git checkout branch-6-0
if $?; then exit; fi
git merge branch-5-6
if $?; then exit; fi
git checkout master
if $?; then exit; fi
git merge branch-6-0

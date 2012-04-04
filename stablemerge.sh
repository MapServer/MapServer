#!/bin/bash

git checkout branch-5-0
if $? -ne 0; then exit; fi
git merge branch-4-10
if $? -ne 0; then exit; fi
git checkout branch-5-2
if $? -ne 0; then exit; fi
git merge branch-5-0
if $? -ne 0; then exit; fi
git checkout branch-5-4
if $? -ne 0; then exit; fi
git merge branch-5-2
if $? -ne 0; then exit; fi
git checkout branch-5-6
if $? -ne 0; then exit; fi
git merge branch-5-4
if $? -ne 0; then exit; fi
git checkout branch-6-0
if $? -ne 0; then exit; fi
git merge branch-5-6
if $? -ne 0; then exit; fi
git checkout master
if $? -ne 0; then exit; fi
git merge branch-6-0

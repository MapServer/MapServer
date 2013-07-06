# -*- coding: utf-8 -*-
import os
import sys

local_file = open('local_mapserver_coverage_html.txt', 'rt')
local_file_lines = local_file.readlines()
local_file.close()

distant_file = open('mapserver_coverage_html.txt', 'rt')
distant_file_lines = distant_file.readlines()
distant_file.close()

remote_dict = {}
remote_paths_dict = set()
for line in distant_file_lines:
    line = line.strip()
    (md5, filename) = line.split()
    dirname = os.path.dirname(filename)
    while dirname != '':
        remote_paths_dict.add(dirname)
        dirname = os.path.dirname(dirname)
    remote_dict[filename] = md5

local_dict = {}
for line in local_file_lines:
    line = line.strip()
    (md5, filename) = line.split()
    local_dict[filename] = md5
    if not (filename in remote_dict and remote_dict[filename] == md5):
        lst_dirs = []
        dirname = os.path.dirname(filename)
        while not (dirname != '' and dirname in remote_paths_dict):
            lst_dirs.insert(0, dirname)
            dirname = os.path.dirname(dirname)
        for dirname in lst_dirs:
            sys.stdout.write('mkdir %s; ' % dirname)
            remote_paths_dict.add(dirname)
        sys.stdout.write('put %s -o %s; ' % (filename, filename))

for filename in remote_dict:
    if filename not in local_dict:
        sys.stdout.write('rm %s; ' % filename)

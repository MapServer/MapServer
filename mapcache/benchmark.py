#! /usr/bin/env python

import commands
import os
import re

def do_ab_call(url,nthreads,reqs):
    cmd="ab -k -c %d -n %d '%s'" % (nthreads,reqs,url)
    print cmd
    summary={}
    ret = commands.getoutput(cmd)
    sList = ret.split(os.linesep)
    for i, line in enumerate(sList):
        if re.match("Requests per second", line) is not None:
            val = line.split()
            summary['reqspersec']=val[3]
        if re.match("Document Length", line) is not None:
            val = line.split()
            summary['size']=val[2]
    return summary

base="http://localhost:8081"
params="LAYERS=test,test3&FORMAT=image%2Fpng&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&STYLES=&EXCEPTIONS=application%2Fvnd.ogc.se_inimage&SRS=EPSG%3A4326&BBOX=-2.8125,47.8125,0,50.625&WIDTH=256&HEIGHT=256"
urls={}
nreqs=400

title="tile merging"
filebase=title

urls['tilecache']="%s/%s?%s" % (base,'tilecache',params)
urls['mapcache best compression']="%s/%s?%s" % (base,'mapcache-best',params)
urls['mapcache default compression']="%s/%s?%s" % (base,'mapcache-default',params)
urls['mapcache fast compression']="%s/%s?%s" % (base,'mapcache-fast',params)
urls['mapcache png quantization']="%s/%s?%s" % (base,'mapcache-pngq',params)
#urls['mapproxy']="http://localhost:8080/service?%s" % (params)

plotfile = open("%s.plot"%(filebase),"w")
datafile = open("%s.dat"%(filebase),"w")

plotfile.write("set terminal pdf\nset key autotitle columnhead\nset output \"%s.pdf\"\nset style data lines\n"%(filebase))
plotfile.write("set xlabel \"concurrent requests\"\n")
plotfile.write("set ylabel \"throughput (requests/sec)\"\n")
plotfile.write("set title \"%s\"\n"%(title))
count=0
for title,url in urls.iteritems():
    if count == 0:
        plotfile.write("plot \"%s.dat\" using 2:xticlabel(1) index 0"%(filebase))
    else:
        plotfile.write(",\"\" using 2 index %d"%(count))
    count += 1
    for nthreads in [1,2,3,4]:
        reqs = min(nthreads,4) * nreqs
        res = do_ab_call(url,nthreads,reqs)
        if nthreads == 1:
            datafile.write("\n\nthreads \"%s (%s bytes)\"\n"%(title,res['size']))
        datafile.write("%d %s\n"%(nthreads,res['reqspersec']))
plotfile.write("\n")


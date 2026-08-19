#!/usr/bin/env python3
# Documents the A21 design intent. The authoritative final LUT is in
# source/clean_integrated_project/spring_curve_a21.h.
def smoothstep(x):
    x=max(0.0,min(1.0,x));return x*x*(3.0-2.0*x)
a20=[round(s*s/127) for s in range(128)]
out=[]
for s,old in enumerate(a20):
    if s<=82:d=0.0
    elif s<=110:d=16.0*smoothstep((s-82)/(110-82))
    else:d=16.0*(1.0-smoothstep((s-110)/(127-110)))
    out.append(min(127,max(old,round(old+d))))
for i in range(0,128,16):print(', '.join(map(str,out[i:i+16])))

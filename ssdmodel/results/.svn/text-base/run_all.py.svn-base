#!/usr/bin/env python3

import os
import sys
import subprocess

def run_cmd(cmd_line, no_run = False):
    fullcommand = "bash -c \"" +  cmd_line + "\""
    #print(fullcommand)
    if not no_run:
        return os.system( fullcommand )
    return None

def create_configuration_file(number_of_cache_lines, number_of_ways, cache_invoke_gap):
    file = open( "../ssd_cache_parameters.h", "w")
    config = """// Config
#define CACHE_SIZE %(number_of_cache_lines)d
#define CACHE_WAY %(number_of_ways)d
#define CACHE_INVOKE_GAP %(cache_invoke_gap)d
""" % locals()
    file.write(config)
    file.close()

def get_name(bench, number_of_cache_lines, number_of_ways, cache_invoke_gap):
    return '%(bench)s %(number_of_ways)d %(number_of_cache_lines)d %(cache_invoke_gap)d' % locals()

def run_ssd(bench, number_of_cache_lines, number_of_ways, cache_invoke_gap):
    trace_folder = '../../../../disksim_trace_files/'
    create_configuration_file(number_of_cache_lines, number_of_ways, cache_invoke_gap)

    res = run_cmd("./force_recompile &> compilation.log")
    if res != 0:
        print('Error 1')
        sys.exit(1)

    cmd_line = "../../src/disksim %(trace_folder)sssd-cache.parv ssd-%(bench)s.outv ascii %(trace_folder)sssd-%(bench)s.trace 0" % locals()

    res = run_cmd(cmd_line + " &> run.log")
    if res != 0:
        print('Error 2')
        sys.exit(1)

    cmd_line = """grep "ssd Response time average:" ssd-%(bench)s.outv | grep -v "#" | cut -f 5- -d" " """ % locals()
    (status, data) = subprocess.getstatusoutput(cmd_line)
    response_time = float(data.strip(' '))

    cmd_line = """grep "System Total Requests" ssd-%(bench)s.outv | grep -v "#" | cut -f 2- -d":" """ % locals()
    (status, data) = subprocess.getstatusoutput(cmd_line)
    total_requests = int(data.strip(' '))

    result = '%(bench)s %(number_of_ways)d %(number_of_cache_lines)d %(cache_invoke_gap)d %(response_time)f %(total_requests)d' % locals()
    print(result)
    file = open('results.log', 'a')
    file.write(result + '\n')
    file.close()

benchs = ['postmark', 'iozone', 'random-writebiased-100k', 'constant-writeonly-100k', 'random-writeonly-100k', 'random-balanced-20k', 'seq-writeonly-20k', 'constant-writeonly-20k', 'random-writebiased-20k', 'random-writeonly-20k', 'thrashtype2-20k']
cache_sizes = [8*256, 16*256, 32*256, 64*256, 128*256, 256*256]
ways = [1, 2, 4]
invoke_gaps = [1, 2, 3, 4, 5, 6]

for bench in benchs:
    for w in ways:
        for cs in cache_sizes:
            for ig in invoke_gaps:
                name = get_name(bench, cs, w, ig)
                cmd = """grep "%(name)s" results.log """ % locals()
                (status, data) = subprocess.getstatusoutput(cmd)
                if status == 0:
                    print (cmd, status, ' [skipped]')
                else:
                    print (cmd, status, ' [running]')
                    run_ssd(bench, cs, w, ig)


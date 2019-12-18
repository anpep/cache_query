#!/usr/bin/env/python3
# coding: utf8
import os
import os.path
import glob
import shlex
import subprocess

ANSI_RESET = "\x1b[0m"
ANSI_CYAN = "\x1b[36m"
ANSI_GREEN = "\x1b[32m"
ANSI_RED = "\x1b[31m"

def _parse_variants(path: str) -> list:
    with open(path, 'r') as f:
        lines = list(filter(lambda l: l.startswith('//!') and 'var ' in l, f.read().splitlines()))
        lines = list(map(lambda l: tuple(l.split('var ')[1].split(':')), lines))
        return list(map(lambda t: (t[0], shlex.split(t[1].strip())), lines))

def _spawn_perf(argv: list) -> subprocess.Popen:
    return subprocess.Popen(['sudo', './cache_query', *argv], close_fds=True,
        stdout=subprocess.PIPE)

def _read_counters(p: subprocess.Popen):
    out = p.stdout.read()
    if p.returncode != 0:
        print('%s[-] cache_query exited with status %d%s' % (ANSI_RED, p.returncode, ANSI_RESET))
        exit(1)
    return dict(map(lambda l: (l[0], int(l[1])), map(lambda l: l.split('='), out.decode().splitlines())))

def main():
    _subprocs = []
    for p in glob.glob('test/*.c'):
        print('[*] %s' % p)
        vars = _parse_variants(p)
        for variant in vars:
            print('[*]   building variant %s%s%s' % (ANSI_CYAN, variant[0], ANSI_RESET))
            _subprocs.append(subprocess.Popen(['cc', *variant[1], '-o',
                '/tmp/' + variant[0], p]))
        for sub in _subprocs:
            sub.wait()
            if sub.returncode != 0:
                print('%s[-] compilation failed (subprocess exited with status %d)%s' %
                    (ANSI_RED, sub.returncode, ANSI_RESET))
                exit(1)
        print('%s[+] compilation finished%s' % (ANSI_GREEN, ANSI_RESET))
        with open('/tmp/perf.dat', 'w') as f:
            _varsubs = []
            for variant in vars:
                _varsubs.append((variant, subprocess.Popen(['sudo', './cache_query',
                    '/tmp/' + variant[0]], close_fds=True, stdout=subprocess.PIPE)))
            for variant_sub in _varsubs:
                variant, sub = variant_sub
                sub.wait()
                counters = _read_counters(sub)
                x_axis = int(variant[0].split('_')[1])
                y_axis = float(counters['cache_misses']) / float(counters['cache_references'])
                print('[*] %s%s%s: %d refs, %d misses (miss rate: %f), %d cycles, %d instructions (IPC: %f)'
                    % (ANSI_CYAN, variant[0], ANSI_RESET,
                    counters['cache_references'], counters['cache_misses'],
                    y_axis, counters['cycles'], counters['instructions'],
                    float(counters['cycles']) / float(counters['instructions'])))
                f.write('%d %f\n' % (x_axis, y_axis))
        with open('/tmp/plot.gnu', 'w') as f:
            f.write('set term png\nset output "/tmp/plot.png"\n')
            f.write('plot "/tmp/perf.dat" using 1:2 w l lt 2 lw 3')
        subprocess.call(['gnuplot', '/tmp/plot.gnu'])
        os.rename('/tmp/plot.png', 'cache_query_' +
            os.path.splitext(os.path.basename(p))[0] + '.png')
        os.unlink('/tmp/plot.gnu')
        os.unlink('/tmp/perf.dat')
        for variant in vars:
            os.unlink('/tmp/' + variant[0])

if __name__ == '__main__':
    main()

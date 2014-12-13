#!/usr/bin/python
#coding:utf-8

import sys
import time
import glob
import marshal
import cPickle
import simplejson as json

libpath = glob.glob('build/lib.*/')
sys.path.append(libpath[0])
print sys.path
import fserial


test_cases = [
    {'a': None},
    {'a': True},
    {'a': False},

    {'a': 1},
    {'a': 9223372036854775807},
    {'a': -9223372036854775808},

    {'a': 9223372036854775807L},
    {'a': -9223372036854775808L},

    {'a': 9223372036854775808L},
    {'a': -9223372036854775809L},

    {'a': 1.0},
    {'a': -1.0},

    {'a': 'b'},

    {'a': None, 'b': True, 'c': False, 'd': 1, 'e': 1.0, 'f': 'x'},

    {
        'hello': None,
        'b': '12312312312',
        'c': 123213213,
        'd': 1,
        'e': 1.0,
        '12312312321': 1123123.21321321312,
        'g': 'adjsakldjaskldjioajdklsadfklasdiofsjakdlsajkldas'
    },
]

for i, c in enumerate(test_cases):
    try:
        x = fserial.dumps(c)
    except Exception, e:
        print >>sys.stderr, "%dth case err: exception[%s] <- %s" % (i, e.message, str(c))
        continue
        
    y = fserial.loads(x)
    if y != c:
        print >>sys.stderr, '%dth case err: %s vs %s => [%s]' % (i, str(c), str(y), x)
    else:
        print >>sys.stderr, '%dth case pass: %s' % (i, str(c))

#performance test
def time_run(foo):
    def wrapper(*args, **kwargs):
        begin_at = time.time()
        foo(*args, **kwargs)
        end_at = time.time()
        print >>sys.stderr, '  time used: %.6f' % (end_at - begin_at)
    return wrapper

n = 5000000
#x = {'a': None, 'b': True, 'c': False, 'd': 1, 'e': 1.0, 'f': 'x'}
x = {
    'hello': None,
    'b': '12312312312',
    'c': 123213213,
    'd': 1,
    'e': 1.0,
    '12312312321': 1123123.21321321312,
    'g': 'adjsakldjaskldjioajdklsadfklasdiofsjakdlsajkldas'
}

fy = fserial.dumps(x)
my = marshal.dumps(x)
cy = cPickle.dumps(x)
jy = json.dumps(x)

@time_run
def test_dumps(name, f):
    print >>sys.stderr, '%s.dumps:' % name
    for i in range(n):
        f(x)

@time_run
def test_loads(name, f, y):
    print >>sys.stderr, '%s.loads:' % name
    for i in range(n):
        f(y)

test_dumps('fserial', fserial.dumps)
test_dumps('marshal', marshal.dumps)
test_dumps('cPickle', marshal.dumps)
#test_dumps('simplejson', json.dumps) #10 times slower than cPickle, forget it

test_loads('fserial', fserial.loads, fy)
test_loads('marshal', marshal.loads, my)
test_loads('cPickle', cPickle.loads, cy)
test_loads('simplejson', json.loads, jy)

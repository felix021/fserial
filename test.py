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
fserial.setbufsize(65536)


test_cases = [
    None, True, False,
    1, -1, 9223372036854775807, -9223372036854775808,
    1.0, -1.0, 11111.11111, -11111.11111,
    9223372036854775807L, -9223372036854775808L, 9223372036854775808L, -9223372036854775809L,
    "hello", "", "world!",
    [1, 2, 3], [1,[2,[3],4],5],
    (1, 2, 3), (1,(2,(3,),4),5),
    (2, 3),
    set([1,2,3]), set([1, "hello", 1.1]), set([1, "hello"]), set([1, (2, 3)]),
    {'a': 1, 2: 'b'}, {'a': 1, 2: {'b': 3}},
    {'a': [None, (True, {False: 4.0}, 1), "!"]},
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
    print >>sys.stderr, '%dth case:' % i, type(c), str(c)
    try:
        x = fserial.dumps(c)
        print >>sys.stderr, "  dumps:", [x]
    except Exception, e:
        print >>sys.stderr, "  dumps err: exception[%s]" % (e.message)
        raise Exception('dumps failed')
        continue
        
    try:
        y = fserial.loads(x)
        print >>sys.stderr, "  loads:", type(y), str(y)
    except Exception, e:
        print >>sys.stderr, "  loads err: exception[%s]" % (e.message)
        raise Exception('loads failed')
        continue

    if c == y:
        print >>sys.stderr, ' pass'
    else:
        print >>sys.stderr, ' fail'
        raise Exception('fail')

#sys.exit(0)

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

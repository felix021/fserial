fserial
=======

simplest {,un}serializer for one-depth-dict with only str keys and str/int/float/None/True/False values

in such cases, this extension runs much faster than builtin python serializers like marshal and cPickle.

Performance test:
=======

you can try test.py yourself.

time consumed for 5,000,000 round of dumps/loads:

    fserial.dumps: time used: 1.434218
    marshal.dumps: time used: 5.686624
    cPickle.dumps: time used: 5.663999
    simplejson.loads: #approximate 10x slower than cPickle/marshal

    fserial.loads: time used: 3.218033
    marshal.loads: time used: 6.762694
    cPickle.loads: time used: 16.427137
    simplejson.loads: time used: 23.069645

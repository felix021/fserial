fserial
=======

simple {,un}serializer for several python builtin objects (None/True/False/int/long/float/str/list/tuple/dict/set)

this extension runs much faster than builtin python serializers like marshal and cPickle.

Notice
=======

This extension uses a fix-length buffer in dumps, which has a default length of 64KB. Call fserial.setbufsize to set a larger length, in case the result may exceed the default value.

Performance test:
=======

you can try test.py yourself.

Time consumed for 5,000,000 rounds of dumps/loads:

    fserial.dumps: 1.581582
    marshal.dumps: 5.548474
    cPickle.dumps: 5.602932
    simplejson.loads: #approximate 10x slower than cPickle/marshal

    fserial.loads: 3.616863
    marshal.loads: 6.749527
    cPickle.loads: 15.953505
    simplejson.loads: 23.159412

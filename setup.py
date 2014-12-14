#!/usr/bin/python
import os
from distutils.core import setup, Extension

#os.putenv("CFLAGS", "-g")

fserial = Extension(
    'fserial',
    sources = ['fserial.c']
)

setup(
    name            = 'fserial',
    version         = '0.0.1',
    author          = 'felix021',
    author_email    = 'felix021@gmail.com',
    description     = 'simple and faster {,un}serializer for most python builtin data objects',
    license         = "Public Domain",
    keywords        = "python extension simple serializer and unserializer",
    url             = "http://github.com/felix021/fserial",
    ext_modules     = [fserial],
#   packages        = ["fserial"]
)

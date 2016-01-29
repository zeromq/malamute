Using the Python binding
========================

This is a ctypes Python binding. You will need to have the Malamute library installed on 
your system as well as the [czmq](http://github.com/zeromq/czmq) library.

The Python binding depends on [czmq Python 
binding](https://github.com/zeromq/czmq/tree/master/bindings/python). You can specify 
its path as follows:

    $ PYTHONPATH="/path/to/czmq.py/ python malamute.py

Usuage
------
    
To use the binding in your python code:

    from malamute import *
    
    client = MlmClient()
    # etc....

To run your code:
    
    $ PYTHONPATH="/path/to/czmq.py/:/path/to/malamute.py/" python malamute.py

If you don't want to specify the PYTHONPATH your options are as follows:

* Make sure the binding is somewhere in standard PYTHONPATH (see sys.path)
* copy the .py files to the working directory
* add the paths in your python code (sys.path.append("/path/to")

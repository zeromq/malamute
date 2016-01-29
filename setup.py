from distutils.core import setup

setup(name='malamute',
	  description="""ZeroMQ Message Broker""",
	  version='0.1',
	  url='git://github.com/zeromq/malamute.git',
	  packages=['malamute'],
	  package_dir={'': 'bindings/python'},
)

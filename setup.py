from distutils.core import setup

setup(name='malamute',
	  description="""ZeroMQ Message Broker""",
	  version='0.1',
	  url='https://github.com/zeromq/malamute',
	  packages=['malamute'],
	  package_dir={'': 'bindings/python'},
)

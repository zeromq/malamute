from malamute import MalamuteClient

def test(addr):
	service = MalamuteClient()
	service.connect(addr, 100, b'service')
	service.set_worker(b'service', b'derps')

	writer = MalamuteClient()
	print("writer.connect")
	writer.connect(addr, 100, b'writer')

	reader = MalamuteClient()
	print("reader.connect")
	reader.connect(addr, 100, b'reader')
	print("reader.set_consumer")
	reader.set_consumer(b'writer', b'foo')
	reader.set_consumer(b'writer', b'bar')

	print("writer.send")
	writer.send(b'writer', b'foo', [b'whoaaa', b'whaaaaaa'])
	writer.send(b'writer', b'bar', [b'whoaaa', b'whaaaaaa'])
	print(reader.recv())
	print(reader.recv())

	reader.sendfor(b'service', b'derps', None, 100, [b'foooooo'])
	reader.sendfor(b'service', b'derps', None, 100, [b'foooooo'])
	reader.sendfor(b'service', b'derps', None, 100, [b'foooooo'])
	print(service.recv())
	print(service.recv())
	print(service.recv())

	service.sendto(b'reader', b'response', None, 100, [b'ok'])
	print(reader.recv())

if __name__ == '__main__':
	# this depends on having a test server running
	test(b'tcp://localhost:9999')

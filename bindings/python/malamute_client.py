if __name__ == "__main__":
    c = MlmClient()
    c.connect("tcp://192.168.1.223:9999", 1000, "PythonTest")
    c.set_consumer("stream", ".+")
    msg = c.recv().popstr()
    while (msg == "hello"):
        print(msg)
        msg = c.recv().popstr()

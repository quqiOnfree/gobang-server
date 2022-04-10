import socket,struct,json,time

s = socket.socket()
s.connect(("127.0.0.1",11451))

ms = json.dumps({"asdsad":"asdasd"},ensure_ascii=False).encode('gb18030')
msg = struct.pack("Q",len(ms))+ms

s.send(b'\1\0\0\0\0\0\0\0a')
print(s.recv(1024))
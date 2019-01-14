#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示http服务提供者操作
http服务可使用常用web库实现，譬如flask,tornado,django等
这里使用简单的BaseHTTPServer.BaseHTTPRequestHandler
'''

import threading
import cppcloud
from cppcloud.provider import ProviderBase
from http.server import HTTPServer, BaseHTTPRequestHandler

listen_port = 8000

class TestHTTPHandler(BaseHTTPRequestHandler):
    #处理GET请求  
    def do_GET(self):
        self.send_response(200)
        #self.send_header("Content-Type", 'application/json; charset=utf-8')
        #self.send_header("Content-Length:", len(buff))
        self.end_headers()
        respmsg = "request GET from " + str(self.client_address) + ": path=" + self.path
        self.wfile.write(respmsg.encode())

    def do_POST(self):
        self.send_response(200)
        datas = self.rfile.read(int(self.headers['content-length']))
        #self.send_header("Content-Type", 'application/json; charset=utf-8')
        #self.send_header("Content-Length", len(buff))
        self.end_headers()
        respmsg = "request POST from " + str(self.client_address) + ": body="
        self.wfile.write(respmsg.encode())
        self.wfile.write(datas)
        
        #if exit: http_server.shutdown()

# 服务注册类，只有调用Regist()方法，消费者才能发现此服务
class HttpProvider(ProviderBase):
    scheme = 'http'
    port = listen_port
    http_path = '/proj'

if __name__ == "__main__":
    if cppcloud.init('vpc2', 4800, svrname="THttpPrvd"):
        http_server = HTTPServer(('', int(listen_port)), TestHTTPHandler)
        http_thread = threading.Thread(target=http_server.serve_forever, name="Http_server")
        http_thread.setDaemon(True)
        http_thread.start()

        HttpProvider.Regist(True)

        input('press any key to exit')
        cppcloud.uninit()
    else:
        print('CloudApp start fail, exit')

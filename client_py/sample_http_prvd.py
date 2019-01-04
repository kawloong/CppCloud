#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
示例演示http服务提供者操作
http服务可使用常用web库实现，譬如flask,tornado,django等
这里使用简单的BaseHTTPServer.BaseHTTPRequestHandler
'''

import threading
from cloudapp import CloudApp
from provider import ProviderBase
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler

listen_port = 8000

class TestHTTPHandler(BaseHTTPRequestHandler):
    #处理GET请求  
    def do_GET(self):
        self.send_response(200)
        #self.send_header("Content-Type", 'application/json; charset=utf-8')
        #self.send_header("Content-Length:", len(buff))
        self.end_headers()
        self.wfile.write("{}")

    def do_POST(self):
        self.send_response(200)
        #self.send_header("Content-Type", 'application/json; charset=utf-8')
        #self.send_header("Content-Length:", len(buff))
        self.end_headers()
        self.wfile.write("{post-resp}")
        
        #if exit: http_server.shutdown()

class HttpProvider(ProviderBase):
    scheme = 'http'
    port = listen_port
    http_path = '/proj'

if __name__ == "__main__":
    cloudapp = CloudApp('vpc2', 4800, svrname="THttpPrvd")
    if cloudapp.start():

        http_server = HTTPServer(('', int(listen_port)), TestHTTPHandler)
        http_thread = threading.Thread(target=http_server.serve_forever, name="Http_server")
        http_thread.setDaemon(True)
        http_thread.start()

        HttpProvider.Regist()

        raw_input('press any key to exit')

        
        cloudapp.shutdown()
        cloudapp.join()
    else:
        print('CloudApp start fail, exit')

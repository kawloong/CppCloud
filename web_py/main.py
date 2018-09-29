#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
'''

from flask import Flask,request,redirect, url_for,render_template
import json
import re
import datetime
import time
import os
import sys
sys.path.append("../")
from client_py.scomm_cli2 import ScommCli2
from client_py.cliconfig import config
from client_py.const import *

gScommCli = None
# http server
app = Flask(__name__)

@app.route('/test')
def tset():
    cmd = request.args.get("cmd")
    resp = gScommCli.request(CMD_TESTING_REQ, {
        "cmd": cmd, "toSvr": 3, "fromSvr": 990})
    return resp

if __name__ == '__main__':
    global gScommCli
    gScommCli = ScommCli2( 
        (config['serv_ip'], config['serv_port']),
        clitype = 20,
        svrid = 990,
        progName = "Web-Ctrl",
        progDesc = "Web-Serv(monitor)"
    )

    if gScommCli.run():
        # app.debug = True
        host = config.get('http_host', '0.0.0.0')
        port = config.get('http_port', 80)

        app.response_class.default_mimetype = 'application/json; charset=utf-8'
        app.run(host=host,port=port) # , threaded=True

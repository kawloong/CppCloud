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
        "cmd": cmd, "toSvr": 990})
    return resp

if __name__ == '__main__':
    global gScommCli
    gScommCli = ScommCli2( 
        ('192.168.228.44', 4803),
        clitype = 20,
        svrid = 992,
        progName = "Web-Ctrl",
        progDesc = "Web-Serv(monitor)"
    )

    if gScommCli.run():
        # app.debug = True
        host = config.get('http_host', '0.0.0.0')
        port = config.get('http_port', 800)

        app.response_class.default_mimetype = 'application/json; charset=utf-8'
        app.run(host=host,port=port) # , threaded=True

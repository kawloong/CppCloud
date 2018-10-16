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

gweb_cli = None
# http server
app = Flask(__name__)

@app.route('/test')
def test():
    cmd = request.args.get("cmd")
    resp = gweb_cli.request(CMD_TESTING_REQ, {
        "cmd": cmd, "toSvr": 990})
    return resp

@app.route('/confname')
def confname():
    return gweb_cli.request(CMD_GETCFGNAME_REQ, {})

@app.route('/getconf')
def qconf():
    return gweb_cli.request(CMD_GETCONFIG_REQ, {
        "file_pattern": request.args.get("file_pattern"),
        "key_pattern": request.args.get("key_pattern"),
        "gt_mtime": int(request.args.get("gt_mtime", 0)),
        "incbase": int(request.args.get("incbase", 0))
    })

@app.route('/bookchange')
def bookchange():
    rsp = gweb_cli.request(CMD_BOOKCFGCHANGE_REQ, {
        "file_pattern": request.args.get("file_pattern"),
        "incbase": int(request.args.get("incbase", 0))
    })
    print("bookchange resp:"+rsp)
    return rsp

@app.route('/setconf', methods=['POST'])
def setconf():
    data = request.get_data()
    #print(type(data))
    print(request.json)
    return gweb_cli.request(CMD_SETCONFIG_REQ, request.json)

def onNotifyMsg(cmdid, seqid, msg):
    print(msg)

if __name__ == '__main__':
    gweb_cli = ScommCli2( 
        ('192.168.228.44', 4800),
        clitype = 20,
        svrid = 990,
        progName = "Web-Ctrl",
        progDesc = "Web-Serv(monitor)"
    )

    gweb_cli.setCmdHandle(CMD_EVNOTIFY_REQ, onNotifyMsg);

    if gweb_cli.run():
        # app.debug = True
        host = config.get('http_host', '0.0.0.0')
        port = config.get('http_port', 80)

        app.response_class.default_mimetype = 'application/json; charset=utf-8'
        app.run(host=host,port=port) # , threaded=True

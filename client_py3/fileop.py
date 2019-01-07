#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

'''
bmsh file transport handle
'''
import gzip
import base64
import os


def file2str(filename, rmTempfile = False):
    ret = 0
    try:
        fsrc = fgz = fb64r = None
        context = ''
        gzfilename = filename+'.gz'
        fsrc = open(filename, 'rb')
        fgz = gzip.open(gzfilename, 'wb')
        fgz.write(fsrc.read())
        fgz.close()

        fb64r = open(gzfilename, 'rb')
        context = base64.b64encode(fb64r.read())
    except BaseException as e:
        print(('call file2str Exception:', filename, e))
        ret = -1
    finally:
        fsrc and fsrc.close()
        fgz and fgz.close() 
        fb64r and fb64r.close()
        if fgz and rmTempfile:
            os.unlink(gzfilename)
    
    return (ret, context)

def str2file(context, filename, rmTempfile = False):
    ret = 0
    try:
        fsrc = fgz = f64 = None
        gzfilename = filename+'.gz'
        gzdata = base64.b64decode(context)
        # print 'write len=', len(gzdata)

        f64 = open(gzfilename, 'wb')
        f64.write(gzdata)
        f64.close()

        fgz = gzip.open(gzfilename, 'rb')
        fsrc = open(filename, 'wb')
        fsrc.write(fgz.read())

    except BaseException as e:
        print((filename, e))
        ret = -2
    finally:
        fsrc and fsrc.close()
        fgz and fgz.close()
        f64 and f64.close()
        if f64 and rmTempfile:
            os.unlink(gzfilename)
    
    return ret


if __name__ == "__main__":
    ret, sss = file2str("city.txt", True)
    if 0 == ret:
        print(('len sss=', len(sss)))
        print((str2file(sss, 'cityN.txt', True)))

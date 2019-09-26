# Based on https://github.com/freedesktop/xorg-xsm/blob/52016a5efdf549ffb2a3fb54ce0d0e4e110957f2/auth.c
#
# Python translation copyright 2019, Egil Moeller
#
# $Xorg: auth.c,v 1.4 2001/02/09 02:05:59 xorgcvs Exp $ */
#
# Copyright 1993, 1998  The Open Group
#
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Except as contained in this notice, the name of The Open Group shall not be
# used in advertising or otherwise to promote the sale, use or other dealings
# in this Software without prior written authorization from The Open Group.

# We use temporary files which contain commands to add/remove entries from
# the .ICEauthority file.

import os
import tempfile
import pysmlib.ice

def write_iceauth(addfp, removefp, entry):
    entry = dict(entry)
    entry[b"auth_data"] = entry[b"auth_data"].hex().encode("ascii")

    addfp.write(b"add %(protocol_name)s \"\" %(network_id)s %(auth_name)s %(auth_data)s\n" % entry)
    removefp.write(b"remove protoname=%(protocol_name)s protodata=\"\" netid=%(network_id)s authname=%(auth_name)s\n" % entry)


def unique_filename(path, prefix, mode):
    fd, name = tempfile.mkstemp(suffix=None, prefix=prefix, dir=path, text=False)
    return os.fdopen(fd, mode), name


# Provide authentication data to clients that wish to connect
MAGIC_COOKIE_LEN=16

def SetAuthentication(listenObjs):
    original_umask = os.umask(0o077)
    path = os.environ.get("SM_SAVE_DIR", None)
    if not path:
        path = os.environ.get("HOME", None)
        if not path:
            path = "."

    addfp, addAuthFile = unique_filename(path, ".xsm", "wb")
    #fcntl(fileno(addfp), F_SETFD, FD_CLOEXEC);

    removefp, remAuthFile = unique_filename(path, ".xsm", "wb")
    #fcntl(fileno(removefp), F_SETFD, FD_CLOEXEC);

    authDataEntries = [{} for i in range(2 * len(listenObjs))]

    for i, listenObj in enumerate(listenObjs):
        authDataEntries[i*2][b"network_id"] = listenObj.IceGetListenConnectionString()
        authDataEntries[i*2][b"protocol_name"] = b"ICE"
        authDataEntries[i*2][b"auth_name"] = b"MIT-MAGIC-COOKIE-1"
        authDataEntries[i*2][b"auth_data"] = pysmlib.ice.PyIceGenerateMagicCookie(MAGIC_COOKIE_LEN)

        authDataEntries[i*2+1][b"network_id"] =  listenObj.IceGetListenConnectionString()
        authDataEntries[i*2+1][b"protocol_name"] = b"XSMP"
        authDataEntries[i*2+1][b"auth_name"] = b"MIT-MAGIC-COOKIE-1"
        authDataEntries[i*2+1][b"auth_data"] = pysmlib.ice.PyIceGenerateMagicCookie(MAGIC_COOKIE_LEN)

        write_iceauth(addfp, removefp, authDataEntries[i*2])
        write_iceauth(addfp, removefp, authDataEntries[i*2+1])

    pysmlib.ice.PyIceSetPaAuthData(authDataEntries)

    addfp.close()
    removefp.close()

    os.umask(original_umask)

    command = "iceauth source %s" % addAuthFile
    os.system(command)

    os.unlink(addAuthFile)

    return remAuthFile


# Free up authentication data.
def FreeAuthenticationData(remAuthFile):
    command = "iceauth source %s" % remAuthFile
    os.system(command)
    os.unlink(remAuthFile)

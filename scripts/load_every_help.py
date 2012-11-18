#!/usr/bin/python

import subprocess, sys, socket, time, os, re, time, smtplib, signal

try:
    pdrootdir = sys.argv[1]
except IndexError:
    print 'only one arg: root dir of pd'
    sys.exit(2)

PORT = 55555
netreceive_patch = '/tmp/.____pd_netreceive____.pd'

def find_pdexe(rootdir):
    # start with the Windows/Mac OS X location
    exe = pdrootdir + '/bin/pd'
    if not os.path.isfile(exe):
        # try the GNU/Linux location
        exe = pdrootdir+'../../bin/pd'
        if not os.path.isfile(exe):
            print "ERROR: can't find pd executable"
            exit
    return os.path.realpath(exe)


def make_netreceive_patch(filename):
    fd = open(filename, 'w')
    fd.write('#N canvas 222 130 454 304 10;')
    fd.write('#X obj 111 83 netreceive ' + str(PORT) + ' 0 old;')
    fd.write('#X obj 111 103 loadbang;')
    fd.write('#X obj 111 123 print netreceive_patch;')
# it would be nice to have this patch tell us when it is closed...
#    fd.write('#X obj 211 160 tof/destroysend pd;')
#    fd.write('#X obj 211 160 closebang;')
#    fd.write('#X obj 211 180 print CLOSE;')
    fd.write('#X connect 1 0 2 0;')
#    fd.write('#X connect 3 0 4 0;')
    fd.close()

def send_to_socket(message):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    try:
        s.connect(('localhost', PORT))
        s.send(message)
        s.close()
    except socket.timeout:
        print "socket timed out while sending"

def send_to_pd(message):
    send_to_socket('; pd ' + message + ';\n')

def open_patch(filename):
    dir, file = os.path.split(filename)
    send_to_pd('open ' + file + ' ' + dir)

def close_patch(filename):
    dir, file = os.path.split(filename)
    send_to_pd('; pd-' + file + ' menuclose')


def launch_pd():
    pdexe = find_pdexe(pdrootdir)
    p = subprocess.Popen([pdexe, '-nogui', '-stderr', '-open', netreceive_patch],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, 
                         close_fds=True);
    line = p.stdout.readline()
    while line != 'netreceive_patch: bang\n':
        line = p.stdout.readline()
    return p

def quit_pd(process):
    send_to_pd('quit')
    time.sleep(1)
    try:
        os.kill(process.pid, signal.SIGTERM)
    except OSError:
        print 'OSError on SIGTERM'
    time.sleep(1)
    try:
        os.kill(process.pid, signal.SIGKILL)
    except OSError:
        print "OSError on SIGKILL"


#---------- list of lines to ignore ----------#
def remove_ignorelines(list):
    ignorelines = [
        'expr, expr~, fexpr~ version 0.4 under GNU General Public License \n',
        'fiddle version 1.1 TEST4\n',
        'sigmund version 0.03\n',
        'pique 0.1 for PD version 23\n',
        'this is pddplink 0.1, 3rd alpha build...\n',
        'beware! this is tot 0.1, 19th alpha build...\n',
        '\n'
        ]
    ignorepatterns = [
        'ydegoyon@free.fr',
        'olaf.matthes@gmx.de',
        'Olaf.*Matthes',
        '[a-z]+ v0\.[0-9]',
        'IOhannes m zm',
        'part of zexy-',
        'based on sync from jMax'
        ]
    for ignore in ignorelines:
        try:
            list.remove(ignore)
        except ValueError:
            pass
    for line in list:
        for pattern in ignorepatterns:
            m = re.search('.*' + pattern + '.*', line)
            while m:
                try:
                    list.remove(m.string)
                    m = re.search('.*' + pattern + '.*', line)
                except ValueError:
                    break
    return list


#---------- main()-like thing ----------#

make_netreceive_patch(netreceive_patch)

logoutput = []
docdir = os.path.join(pdrootdir, 'doc')
for root, dirs, files in os.walk(docdir):
    for name in files:
        m = re.search(".*\.pd$", name)
        if m:
            patchoutput = []
            patch = os.path.join(root, m.string)
            p = launch_pd()
            try:
                open_patch(patch)
                time.sleep(5)
                close_patch(patch)
                quit_pd(p)
            except socket.error:
                patchoutput.append('socket.error')                 
            while True:
                line = p.stdout.readline()
                m = re.search('EOF on socket', line)
                if not m and line:
                    patchoutput.append(line) 
                else:
                    break
            patchoutput = remove_ignorelines(patchoutput)
            if len(patchoutput) > 0:
#                print 'found log messages: ' + patch
                logoutput.append('\n\n__________________________________________________\n')
                logoutput.append('loading: ' + patch + '\n')
#                logoutput.append('--------------------------------------------------\n')
                logoutput += patchoutput
#                for line in patchoutput:
#                    print '--' + line + '--'

now = time.localtime(time.time())
date = time.strftime('20%y-%m-%d', now)
datestamp = time.strftime('20%y-%m-%d_%H.%M.%S', now)

outputfilename = 'load_every_help_' + socket.gethostname() + '_' + datestamp + '.log'
outputfile = '/tmp/' + outputfilename
fd = open(outputfile, 'w')
fd.writelines(logoutput)
fd.close()


# make the email report
fromaddr = 'pd@pdlab.idmi.poly.edu'
toaddr = 'hans@at.or.at'
mailoutput = []
mailoutput.append('From: ' + fromaddr + '\n')
mailoutput.append('To: ' + toaddr + '\n')
mailoutput.append('Subject: load_every_help ' + datestamp + '\n\n\n')
mailoutput.append('______________________________________________________________________\n\n')
mailoutput.append('Complete log:\n')
mailoutput.append('http://autobuild.puredata.info/auto-build/' + date + '/'
                  + outputfilename + '\n')


# upload the log file to the autobuild website
rsyncfile = 'rsync://128.238.56.50/upload/' + date + '/logs/' + outputfilename
try:
    p = subprocess.Popen(['rsync', '-ax', outputfilename, rsyncfile],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT).wait()
except:
    mailoutput.append('rsync upload of the log failed!\n')
    mailoutput.append(''.join(p.stdout.readlines()))



mailoutput.append('______________________________________________________________________\n\n')
server = smtplib.SMTP('in1.smtp.messagingengine.com')
server.sendmail(fromaddr, toaddr, ''.join(mailoutput + logoutput))
server.quit()

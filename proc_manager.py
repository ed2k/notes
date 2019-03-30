#!/usr/bin/env python
import os
import sys 
import fcntl
import errno
import signal
import subprocess

import glob
import shutil
import hashlib
import importlib
import subprocess
import traceback
from multiprocessing import Process

import time

class cloudlog(object):
  @staticmethod
  def info(s):
    print(s)
  @staticmethod
  def critical(s):
    print('CRITICAL: {}'.format(s))
  @staticmethod
  def warning(s):
    print('WARN: {}'.format(s))

class crash(object):
  @staticmethod
  def capture_exception():
    print('todo capture exeception')


from threading import Thread
from Queue import Queue, Empty

class NonBlockingStreamReader:

    def __init__(self, stream):
        '''
        stream: the stream to read from.
                Usually a process' stdout or stderr.
        '''

        self._s = stream
        self._q = Queue()

        def _populateQueue(stream, queue):
            '''
            Collect lines from 'stream' and put them in 'quque'.
            '''
            self.running = True
            while self.running:
                line = stream.readline()
                if line:
                    queue.put(line)
                else:
                    time.sleep(1)

        self._t = Thread(target = _populateQueue,
                args = (self._s, self._q))
        self._t.daemon = True
        self._t.start() #start collecting lines from the stream

    def readline(self, timeout = None):
        return self._q.get(block = timeout is not None,
                    timeout = timeout)

    def stop(self):
        self.running = False

a = '--chain etg --reserved-peers mp.txt  --tracing=on --fat-db=on -d /mnt/volume_nyc1_04/io.parity.ethereum/ --port 32800 --stratum-port=30100 --stratum --stratum-interface=0.0.0.0 --author 0xf0D06A0e485D0B47ca2418041AdD2Fb1838a601e --log-file ete.log'
CMDS = a.split()
# comment out anything you don't want to run
managed_processes = { 
  "parityete": (".", ["./pete-v1.8.1"]+CMDS),
  "parityete2": (".", ["./pete-v1.12"]+CMDS),
}

running = {}
# processes to end with SIGINT instead of SIGTERM
interrupt_processes = []
unkillable_processes = []

BASEDIR = os.getcwd()

def get_running():
  return running


# ****************** process management functions ******************
def launcher(proc, gctx):
  sys.stdout = open(str(os.getpid()) + ".out", "w")
  try:
    # import the process
    mod = importlib.import_module(proc)

    # rename the process
    setproctitle(proc)

    # exec the process
    mod.main(gctx)
  except KeyboardInterrupt:
    cloudlog.warning("child %s got SIGINT" % proc)
  except Exception:
    # can't install the crash handler becuase sys.excepthook doesn't play nice
    # with threads, so catch it here.
    crash.capture_exception()
    raise

def nativelauncher(pargs, cwd):
  # exec the process
  os.chdir(cwd)

  # because when extracted from pex zips permissions get lost -_-
  os.chmod(pargs[0], 0o700)
  print('nl',pargs)
  os.execvp(pargs[0], pargs)


def start_managed_process(name):
  return
  if name in running or name not in managed_processes:
    return
  proc = managed_processes[name]
  if isinstance(proc, str):
    cloudlog.info("starting python %s" % proc)
    running[name] = Process(name=name, target=launcher, args=(proc, gctx))
  else:
    pdir, pargs = proc
    cwd = os.path.join(BASEDIR, pdir)
    cloudlog.info("starting process {} {}".format(name, pargs))
    running[name] = Process(name=name, target=nativelauncher, args=(pargs, cwd))
  running[name].start()

def kill_managed_process(name):
  return
  if name not in running or name not in managed_processes:
    return
  cloudlog.info("killing %s" % name)

  if running[name].exitcode is None:
    if name in interrupt_processes:
      os.kill(running[name].pid, signal.SIGINT)
    else:
      running[name].terminate()

    # give it 5 seconds to die
    running[name].join(5.0)
    if running[name].exitcode is None:
      if name in unkillable_processes:
        cloudlog.critical("unkillable process %s failed to exit! rebooting in 15 if it doesn't die" % name)
        running[name].join(15.0)
        if running[name].exitcode is None:
          cloudlog.critical("FORCE REBOOTING ...!")
          os.system("date >> /sdcard/unkillable_reboot")
          #os.system("reboot")
          raise RuntimeError
      else:
        cloudlog.info("killing %s with SIGKILL" % name)
        os.kill(running[name].pid, signal.SIGKILL)
        running[name].join()

  cloudlog.info("%s is dead with %d" % (name, running[name].exitcode))
  del running[name]


def cleanup_all_processes(signal, frame):
  print("caught ctrl-c %s %s" % (signal, frame))

  #pm_apply_packages('disable')

  for name in list(running.keys()):
    kill_managed_process(name)
  #cloudlog.info("everything is dead")


def get_status():
  with open('ete.log') as f:
    f.seek(-1, os.SEEK_END)
    r = NonBlockingStreamReader(f)
    while True:
      line = r.readline(30)
      if line is None or line == '':
         return ''
      pos = line.find("Syncing #")
      print('line: ', line)
      if pos > 0 and line[pos+9:] != '':
         n = line[pos+9:].split()[0]
         print(pos, n)
         r.stop()
         return n

import random
def get_message():
  return ['check', '']


def manager_thread():
  ete = 'parityete'
  oldsts = ''
  while True:
    #cfd = os.path.dirname(os.path.realpath(__file__))
    cfd = os.path.realpath(__file__)
    cwd = os.getcwd()
    print('{} at {}'.format(cfd, cwd))
    msg =  get_message()
    print(msg)
    #managed_processes[ete][1][-1] = msg[1]
    if oldsts == '':
      start_managed_process(ete)
    t = random.randint(1, 16)
    time.sleep(t)
    sts = get_status()
    if sts == oldsts:
      kill_managed_process(ete)
      ete = 'parityete2'
    elif ete is 'parityete2':
      kill_managed_process(ete)
      ete = 'parityete'
    oldsts = sts
    time.sleep(3)


def main():
  # SystemExit on sigterm
  signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(1))

  try:
    manager_thread()
  except Exception:
    traceback.print_exc()
    crash.capture_exception()
  finally:
    cleanup_all_processes(None, None)


if __name__ == "__main__":
  main()
  # manual exit because we are forked
  sys.exit(0)


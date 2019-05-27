# -*- coding: utf-8 -*-
# @Author: lorenzo
# @Date:   2018-01-26 09:28:27
# @Last Modified by:   m.cipriani
# @Last Modified time: 2019-05-15 18:41:11

"""
.. module:: s7

*****************************************
Siemens S7 Communication Protocol Library
*****************************************

The Zerynth Siemens S7 Communication Protocol Library can be used to ease the connection to a Siemens Programmable Logic Controller (PLC) hosting an S7 Server instance.

It allows to make your device act as an S7 client which can connect to the server and start interacting with it.

.. important:: Siemens S7 Communication Protocol Library is supported only for ESP32 devices.

The library instantiates a :class:`Client` object when :func:`init` function is called, the object is then accessible through :code:`client` variable: ::

    import s7

    s7.init()
    s7.client.connect(my_addr)

    """

# Area ID
S7AreaPE = 0x81 # Process Inputs
S7AreaPA = 0x82 # Process Outputs
S7AreaMK = 0x83 # Merkers
S7AreaDB = 0x84 # DB
S7AreaCT = 0x1C # Counters
S7AreaTM = 0x1D # Timers

# Different areas have different 'word' size, automatically handled by the module
# PE/PA/MK/DB -> 1 Byte for 'word'
# Conunters/Timers -> 2 Bytes for 'word'

@native_c("s7_Cli_Create", ["csrc/s7_ifc.c", "#csrc/misc/zstdlib.c","libsnap7.rvo"])
def cli_create():
    pass

@native_c("s7_Cli_Connect", [])
def cli_connect(addr, rack=0, slot=2):
    pass

@native_c("s7_Cli_GetCpuInfo", [])
def cli_getcpuinfo():
    pass

@native_c("s7_Cli_ReadArea", [])
def cli_readarea(area, dbnumber, start, amount):
    pass

@native_c("s7_Cli_WriteArea", [])
def cli_writearea(area, dbnumber, start, buf):
    pass

@native_c("s7_Cli_ReadMultiVars", [])
def cli_readmultivars(items):
    pass

@native_c("s7_Cli_WriteMultiVars", [])
def cli_writemultivars(items):
    pass

class Client:
    """
===============
The Client class
===============

.. class:: Client()

        Create an S7 client.
        Since only one client can be active, creation is automatically handled by :func:`init` module function.

    """
    def __init__(self):
        pass
    def connect(self, addr, rack=0, slot=2):
        """
.. method:: connect(addr, rack=0, slot=2)

        :param addr: S7 server address
        :param rack: S7 server rack
        :param slot: S7 server slot

        Connect to S7 server.

        """
        return cli_connect(addr, rack, slot)
    def getcpuinfo(self):
        return cli_getcpuinfo()
    def readarea(self, area, dbnumber, start, amount):
        """
.. method:: readarea(area, dbnumber, start, amount)

        :param area: S7 server area to read
        :param dbnumber: S7 server db to read, ignored if :code:`area != S7AreaDB`
        :param start: read offset
        :param amount: read amount as number of words to read, note that different areas have different wordsize (automatically handled by the library)

        Read an S7 server area.
        Returns a :code:`bytes` or a :code:`shorts` object whether chosen area has a wordsize of a single byte or two bytes.

        """
        return cli_readarea(area, dbnumber, start, amount)
    def writearea(self, area, dbnumber, start, buf):
        """
.. method:: writearea(area, dbnumber, start, buf)

        :param area: S7 server area to read
        :param dbnumber: S7 server db to read, ignored if :code:`area != S7AreaDB`
        :param start: write offset
        :param buf: buffer to write, must be a :code:`bytarray` or a :code:`shortarray` whether chosen area has a wordsize of a single byte or two bytes.

        Write :samp:`buf` to an S7 server area.

        """
        return cli_writearea(area, dbnumber, start, buf)
    def readmultivars(self, items):
        """
.. method:: readmultivars(items)

        :param items: read descriptors

        Perform multiple reads within a single call.
        A tuple of read descriptors must be passed, containing tuples describing each read step: ::

            items = ((s7.S7AreaDB, 1, 16, 10), (s7.S7AreaDB, 2, 16, 10))
            bufs = s7.client.readmultivars(items)

        The above example defines a tuple :code:`item` containig two descriptors, the first one to read from DB area, from DB number 1, with an offset of 16, and amount of 10.
        The second to perform the same operation but on DB number 2.

        As it is clear in the example, descriptor params have to be placed in the same order (and with the same meaning) expected by :meth:`readarea` method.

        Returns a tuple of :code:`bytes` or :code:`shorts` objects each corresponding to a single read result.

        """
        return cli_readmultivars(items)
    def writemultivars(self, items):
        """
.. method:: writemultivars(items)

        :param items: write descriptors

        Perform multiple writes withing a single call.
        A tuple of write descriptors must be passed, containing tuples describing each write step: ::

            items = ((s7.S7AreaDB, 1, 16, bytes([1,2,3,4,5,6,7,8])), (s7.S7AreaDB, 2, 16, bytes([1,2,3,4,5,6,7,8])))
            s7.client.writemultivars(items)

        The above example defines a tuple :code:`item` containig two descriptors, the first one to write to DB area, DB number 1, with an offset of 16, the sequence of bytes :code:`[1,2,3,4,5,6,7,8]`.
        The second to perform the same operation but on DB number 2.

        As it is clear in the example, descriptor params have to be placed in the same order (and with the same meaning) expected by :meth:`writearea` method.        

        """
        return cli_writemultivars(items)

client = None
def init():
    """
=================
Library Functions
=================

.. function:: init()

    Init module, after this call :samp:`client` global variable is available containing a :class:`Client` instance.

    """
    global client

    # __builtins__.__default_net["sock"][0].enable_c_sockets()
    client = Client()
    cli_create()

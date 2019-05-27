.. module:: s7

*****************************************
Siemens S7 Communication Protocol Library
*****************************************

The Zerynth Siemens S7 Communication Protocol Library Library can be used to ease the connection to a Siemens Programmable Logic Controller (PLC) hosting an S7 Server instance.

It allows to make your device act as an S7 client which can connect to the server and start interacting with it.

The library instantiates a :class:`Client` object when :func:`init` function is called, the object is then accessible through :code:`client` variable: ::

    import s7

    s7.init()
    s7.client.connect(my_addr)

    
===============
The Client class
===============

.. class:: Client()

        Create an S7 client.
        Since only one client can be active, creation is automatically handled by :func:`init` module function.

    
.. method:: connect(addr, rack=0, slot=2)

        :param addr: S7 server address
        :param rack: S7 server rack
        :param slot: S7 server slot

        Connect to S7 server.

        
.. method:: readarea(area, dbnumber, start, amount)

        :param area: S7 server area to read
        :param dbnumber: S7 server db to read, ignored if :code:`area != S7AreaDB`
        :param start: read offset
        :param amount: read amount as number of words to read, note that different areas have different wordsize (automatically handled by the library)

        Read an S7 server area.
        Returns a :code:`bytes` or a :code:`shorts` object whether chosen area has a wordsize of a single byte or two bytes.

        
.. method:: writearea(area, dbnumber, start, buf)

        :param area: S7 server area to read
        :param dbnumber: S7 server db to read, ignored if :code:`area != S7AreaDB`
        :param start: write offset
        :param buf: buffer to write, must be a :code:`bytarray` or a :code:`shortarray` whether chosen area has a wordsize of a single byte or two bytes.

        Write :samp:`buf` to an S7 server area.

        
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

        
.. method:: writemultivars(items)

        :param items: write descriptors

        Perform multiple writes withing a single call.
        A tuple of write descriptors must be passed, containing tuples describing each write step: ::

            items = ((s7.S7AreaDB, 1, 16, bytes([1,2,3,4,5,6,7,8])), (s7.S7AreaDB, 2, 16, bytes([1,2,3,4,5,6,7,8])))
            s7.client.writemultivars(items)

        The above example defines a tuple :code:`item` containig two descriptors, the first one to write to DB area, DB number 1, with an offset of 16, the sequence of bytes :code:`[1,2,3,4,5,6,7,8]`.
        The second to perform the same operation but on DB number 2.

        As it is clear in the example, descriptor params have to be placed in the same order (and with the same meaning) expected by :meth:`writearea` method.        

        
=================
Library Functions
=================

.. function:: init()

    Init module, after this call :samp:`client` global variable is available containing a :class:`Client` instance.

    

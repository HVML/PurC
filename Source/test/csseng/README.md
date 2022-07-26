CSSEng testcases
===============

Testcases for CSSEng are self-contained binaries which test various parts
of the CSS library. These may make use of external data files to drive
the testing.

Testcase command lines
----------------------

Testcase command lines are in a unified format, thus:

 	<aliases_file> [ <data_file> ]

The aliases file parameter will always be specified (as it is required for
the library to work at all).

The data file parameter is optional and may be provided on a test-by-test
basis.

Testcase output
---------------

Testcases may output anything at all to stdout. The final line of the
output must begin with either PASS or FAIL (case sensitive), indicating
the success status of the test.

Test Index
----------

In the test sources directory, is a file, named INDEX, which provides an
index of all available test binaries. Any new test applications should be
added to this index as they are created.

The test index file format is as follows:

	file         = *line

	line         = ( entry / comment / blank ) LF

	entry        = testname 1*HTAB description [ 1*HTAB datadir ]
	comment      = "#" *non-newline
	blank        = 0<OCTET>

	testname     = 1*non-reserved
	description  = 1*non-reserved
	datadir      = 1*non-reserved

	non-newline  = VCHAR / WSP
	non-reserved = VCHAR / SP

Each entry contains a mandatory binary name and description followed by
an optional data directory specifier. The data directory specifier is
used to state the name of the directory containing data files for the
test name. This directory will be searched for within the "data"
directory in the source tree.

If a data directory is specified, the test binary will be invoked for
each data file listed within the data directory INDEX, passing the
filename as the second parameter (<data_file>, above).

Data Index
----------

Each test data directory contains a file, named INDEX, which provides an
index of all available test data files.

The data index file format is as follows:

	file         = *line

	line         = ( entry / comment / blank ) LF

	entry        = dataname 1*HTAB description
	comment      = "#" *non-newline
	blank        = 0<OCTET>

	dataname     = 1*non-reserved
	description  = 1*non-reserved

	non-newline  = VCHAR / WSP
	non-reserved = VCHAR / SP

Each entry contains a mandatory data file name and description.

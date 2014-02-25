::
:: 4browsers.bat
::
:: This is a test batch file for TitleWait which launches five browsers in sequence,
:: running a small test page.
::
:: For more information on TitleWait, see http://titlewait.hostilefork.com
::


:: Path to titlewait executable to use
:: Note: No spaces around the = in batch files!!!
SET titlewait=..\Debug\TitleWait.exe


:: Delete previous screen snapshots
DEL snapshots\*.bmp


:: Do you want it to be verbose?  yes or no... (or on or off...)
SET verbosity="on"

:: FIREFOX ::

%titlewait% ^
	--regex="Complicated" ^
	--program="C:\Program Files\Mozilla Firefox\firefox" ^
	--args="-new-window MyComplicatedPage.html" ^
	--width=300 ^
	--height=300 ^
	--x=200 ^
	--y=200 ^
	--verbose=%verbosity% ^
	--closeonmatch=true ^
	--regexsnapshot="snapshots\firefox-success.bmp" ^
	--crashsnapshot="snapshots\firefox-crash.bmp" ^
	--timeoutsnapshot="snapshots\firefox-timeout.bmp" ^
	--timeout=30


:: INTERNET EXPLORER ::

%titlewait% ^
	--regex="Complicated" ^
	--program="C:\Program Files\Internet Explorer\iexplore" ^
	--args="file:L:\tests\MyComplicatedPage.html" ^
	--width=300 ^
	--height=300 ^
	--x=200 ^
	--y=200 ^
	--verbose=%verbosity% ^
	--closeonmatch=true ^
	--regexsnapshot="snapshots\iexplore-success.bmp" ^
	--crashsnapshot="snapshots\iexplore-crash.bmp" ^
	--timeoutsnapshot="snapshots\iexplore-timeout.bmp" ^
	--timeout=30


:: SAFARI ::

%titlewait% ^
	--regex="Complicated" ^
	--program="C:\Program Files\Safari\safari" ^
	--args="-url MyComplicatedPage.html" ^
	--width=300 ^
	--height=300 ^
	--x=200 ^
	--y=200 ^
	--verbose=%verbosity% ^
	--closeonmatch=true ^
	--regexsnapshot="snapshots\safari-success.bmp" ^
	--crashsnapshot="snapshots\safari-crash.bmp" ^
	--timeoutsnapshot="snapshots\safari-timeout.bmp" ^
	--timeout=30


:: OPERA ::
:: Note that opera ignores -nosession on windows
:: You need to specifically go into preferences and tell it to startup clean

%titlewait% ^
	--regex="Complicated" ^
	--program="C:\Program Files\Opera\19.0.1326.63\opera" ^
	--args="MyComplicatedPage.html" ^
	--width=300 ^
	--height=300 ^
	--x=200 ^
	--y=200 ^
	--verbose=%verbosity% ^
	--closeonmatch=true ^
	--regexsnapshot="snapshots\opera-success.bmp" ^
	--crashsnapshot="snapshots\opera-crash.bmp" ^
	--timeoutsnapshot="snapshots\opera-timeout.bmp" ^
	--timeout=30
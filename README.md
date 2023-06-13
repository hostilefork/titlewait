![TitleWaitlogo](https://raw2.github.com/hostilefork/titlewait/master/titlewait-logo.jpg)

TitleWait is a program automation and testing tool (currently just for Windows, tested on XP through Windows8).  It can be used to launch a program and then watches for optional conditions to close the program...most notably when the title of the window changes.

Many programs--even those with limited scriptability by the user--can have their window titles dynamically affected by the user content loaded.  One particularly relevant example is the ability to use JavaScript to change the title bar of all major web browsers (when their tab is in the foreground).  This can generate a signal, for instance that tests have run successfully without hitting an error:

    <script type="text/javascript">
        document.title = 'Tests Successful';
    </script>

You can then use TitleWait to watch for the appearance of this string as part of the title bar of any of your windows.  Browsers generally add their own text, but will always have the page title as a part of the title bar.  You can use regular expressions to define the condition

    titlewait --program="iexplore.exe" --regex=".*Tests Successful.*"

Thus, using no other interprocess communication methods, it's possible to use the title to "signal" this small program that a certain condition has been reached.  It will not terminate until that happens, unless you specify a timeout.  As an added bonus, TitleWait can take screen snapshots of the application's window or the whole screen in the case of a crash, timeout, or title match.  You can also set the position of the launched process's window on the screen.

There are many options and return codes to the shell, which you can get by running the program with "--help" (or with no arguments):

    TitleWait (c) 2008-2014 HostileFork.com
    See http://titlewait.hostilefork.com

    Options are like --verbose=1 or --program="foo.exe"
    For all options that are strings, enclose in quotes.
    To embed quotes in those strings, use \"
    For booleans, use on/true/yes/1 or off/false/no/0

    Tip: Long command line? Use ^ to join lines

    OPTION LIST:

      --help
        Show this help information

      --program
        Path to the program to run (required)

      --args
        Arguments to the program

      --regex
        The regular expression you want to search the title for

      --frequency
        How often to check the regex on the title (in seconds, 1 is default)

      --regexsnapshot
        Path to screen snapshot to take if expression matches title (.BMP)

      --closeonmatch
        Send a window close message to the application if regex matches

      --searchall
        Search windows of all processes for regex, not just those this spawns

      --timeout
        Number of seconds the program run before closing it due to timeout

      --timeoutsnapshot
        Path to screen snapshot to take if timeout period elapses (.BMP)

      --crashsnapshot
        Path to screen snapshot to take if program crashes (.BMP)

      --defer
        If the program is already running, wait on execution of new instance

      --x
        X position to ask program window to move to

      --y
        Y position to ask program window to move to

      --width
        Requested width of program window

      --height
        Requested height of program window

      --verbose
        Verbose debugging mode

      --shutdownevent
        Internal use only, passed to nested executive for debug control

    RETURN CODE LIST:

      0 => Success
      1 => Bug! Report to https://github.com/hostilefork/titlewait/issues
      2 => Invoked with bad command line arguments
      3 => No program to run was supplied with --program
      4 => A timeout was specified and program didn't exit before timeout
      5 => Attempt to close the window normally failed, had to terminate
      6 => The spawned process crashed
      7 => The spawned process closed itself before match or timeout
      8 => Running with --defer option and user canceled instead of waiting

When I came up with the idea of using the window title to signal a testing tool from JavaScript, I thought this would be pretty easy.  That was back in 2008, and I found that getting the precise behaviors I wanted were not as simple.  In trying to get a smooth experience, I wound up having to implement TitleWait as as a debugger (using the same APIs that programs like Visual Studio use) to get greater control over spawned processes.

I'd be very interested to know if someone could reproduce these results using [AutoHotkey](http://en.wikipedia.org/wiki/AutoHotkey) (GPL) or [AutoIt](http://en.wikipedia.org/wiki/AutoIt) (free, closed), which I found out about a couple years later.  Though I'd suspect they wouldn't handle some of these cases.

What is tricky in particular is the graceful handling of dealing with programs that are implemented in terms of a launcher and child processes, such as many browsers today.  Being a debugger makes it easier to stay in the driver's seat and get notifications as processes and threads are started and stopped.  So even if you called a program that launched and returned to the shell immediately (like IEXPLORE.EXE), a script using TitleWait can hold up until the processes doing the real work have all actually exited.

In any case, the source code was published in 2013 "in the hope that it might be useful".  GPLv3 license.

*Note: I don't like programming to flat Win32 API any more than anyone else does, but it simplifies the dependency structures.  I can't count the number of dumb little utilities like this I've downloaded which have one simple function but then won't run because they're missing a DLL or something.*

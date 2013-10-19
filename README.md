TitleWait is a program automation and testing tool (currently just for Windows, tested on XP through Windows8).  It can be used to launch a program and then watches for optional conditions to close the program...most notably what the title text of the window is.

Many programs--even those with limited scriptability by the user--can have their window titles dynamically affected by the user content loaded.  One particularly relevant example is the ability to use JavaScript to change the title bar of all major web browsers (when their tab is in the foreground).  This can generate a signal, for instance that tests have run successfully without hitting an error:

    <script type="text/javascript">
        document.title = 'Tests Successful';
    </script>

You can then use TitleWait to watch for the appearance of this string as part of the title bar of any of your windows.  Browsers generally add their own text, but will always have the page title as a part of the title bar.  You can use regular expressions to define the condition

	titlewait --program="iexplore.exe" --regex=".*Tests Successful.*" --close=true

Thus, using no other interprocess communication methods, it's possible to use the title to "signal" this small program that a certain condition has been reached.  It will not terminate until that happens, unless you specify a timeout.  As an added bonus, TitleWait can take screen snapshots of the application's window or the whole screen in the case of a crash, timeout, or title match.  You can also set the position of the launched process's window on the screen.

When I came up with the idea of using the window title to signal a testing tool from JavaScript, I thought this would be pretty easy.  That was back in 2008, and I found that getting the precise behaviors I wanted were not as simple.  In trying to get a smooth experience, I wound up having to implement TitleWait as as a debugger (using the same APIs that programs like Visual Studio use) to get greater control over spawned processes.  I'd be very interested to know if someone could reproduce these results using something like AutoHotkey (GPL) or AutoIt (free, closed), which I found out about later.

In any case, the source code is being shared in 2013 "in the hope that it might be useful".  GPLv3 license.

*Note: I don't like programming to flat Win32 API any more than anyone else does, but it simplifies the dependency structures.  I can't count the number of dumb little utilities like this I've downloaded which have one simple function but then won't run because they're missing a DLL or something.*
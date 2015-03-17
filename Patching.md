# Note #
This page is not finished yet, I intend to post sample code and more information.

# Introduction #

When developing ShutdownGuard, I soon realized that you can't block shutdowns by responding to the [WM\_QUERYENDSESSION](http://msdn.microsoft.com/en-us/library/aa376890%28VS.85%29.aspx) message if the caller to the [ExitWindowsEx()](http://msdn.microsoft.com/en-us/library/aa376868%28VS.85%29.aspx) function specifies the flag EWX\_FORCE to force shutdown.
To resolve this, I found out that you can redirect Windows API calls with a couple of methods. Redirecting ExitWindowsEx() would allow me to simply discard all calls, even if the force flag was specified.
This would also make ShutdownGuard be able to check if the system is shutting down or rebooting (WM\_QUERYENDSESSION does not contain that information).

After reading a bit about it, I chose to investigate two ways to redirect API calls:
  * .local dll redirect
  * IAT patching

# .local dll redirect #

This method works by making a file in the same directory as the .exe file, but with  .local appended to the filename.
When you have created this file, the program will always try to load dll files from the directory it lives in before loading it from the Windows directory etc.

So after creating a .local file, you could make your own User32.dll that intercepts calls to ExitWindowsEx(), and simply forwards all other calls to the real User32.dll.

The problem with this is that the User32.dll file will not be very portable (different version of User32.dll contains different declarations and functions), and you would have to put the .local file and the custom User32.dll next to **every** executable that tries to make calls to ExitWindowsEx().
Due to this I decided that this method would be insufficient for use in ShutdownGuard.

# IAT patching #

IAT (Import Address Table) patching works by replacing the pointer to the function you want to patch with your own function. Then every time the process tries to call that function, your function would be called instead. This requires you to alter the memory of the process in which you want to redirect an API, which can be dangerous, but if you want to patch API calls, this seems to be the best way to do it.

IAT patching doesn't prevent applications from using GetProcAddress() to look up the address and calling it manually, unless you patch GetProcAddress() as well. :)

# Sources #

Several websites helped me accomplish this. Unfortunately sample code for the IAT patching is not widely available from Microsoft, so I almost didn't get any information from them.

  * [.local redirection on MSDN](http://msdn.microsoft.com/en-us/library/aa375142%28VS.85%29.aspx)
  * [Paper mostly about .local dll redirect](http://www.milw0rm.com/papers/105)
  * [Hooking Windows API by Holy\_Father](http://vx.netlux.org/29a/29a-7/Articles/29A-7.019)
  * [API Spying by Yariv Kaplan](http://www.internals.com/articles/apispy/apispy.htm)
  * [Intercepting API calls on CodeProject](http://www.codeproject.com/KB/system/hooksys.aspx)
  * [Injecting dll on CodeProject](http://www.codeproject.com/KB/threads/winspy.aspx)
  * [Code and binaries for injecting a dll](http://www.quantumg.net/injectdll.php)
  * [Thread about IAT patching](http://www.criticalsecurity.net/index.php?showtopic=2397)
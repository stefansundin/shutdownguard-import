# Introduction #

ShutdownPatcher is a separate program from ShutdownGuard. It is an attempt to solve the types of shutdowns that ShutdownGuard can't stop. It does this by patching other processes (overwriting parts of their memory), so that when they attempt to call `ExitWindowsEx()`, the call is intercepted and stopped.

Note that I consider this patcher to be **experimental**. I have developed it for research purposes and I do not consider it to be very stable. **Use at your own risk!**

Read more about patching on this wiki page: [Patching](Patching.md).


# Stability #

Writing in the memory of other processes is always dangerous, so note that there are a slim chance of crashing the other process. And if you attempt to patch a system process, you may crash the entire system and get a bluescreen.

This is also the first time I have programmed something like this, so use it at your own risk.

Also note that because patching other processes is a technique often used by viruses, so there is a big chance that antivirus scanners will mistake ShutdownPatcher for a virus. This is a false positive, of course, and is caused by antivirus heuristics.


# 32-bit and 64-bit #

There is a 32-bit version that is able to patch 32-bit processes only. There is also a 64-bit version that is intended to patch both 32-bit and 64-bit processes. For reasons unknown to me it is only able to patch 64-bit processes. Therefore you might need to run both versions simultaneously.


# Proofness #

**ShutdownPatcher is not bullet-proof!** If a program **really** wants to work around this, it can do so very easily. ShutdownPatcher only patches the executable, not the modules (DLLs) it has loaded.

Note that many error messages from ShutdownPatcher will only be shown when running in debug mode (you can do that by compiling from source yourself). Therefore patching program might fail even if you don't see any error message.

Since there are no API that can tell ShutdownPatcher when new processes appear, ShutdownPatcher will check every 5 seconds if there are any new processes that needs to be patched. A process can thus spawn and issue a forced shutdown before ShutdownPatcher notices it.


# What happens when a shutdown is blocked? #

When the `ExitWindowsEx()` call is intercepted, the process sends a message to `ShutdownPatcher.exe`, telling it that a shutdown has been blocked. ShutdownPatcher will then display a balloon tip notifying you that a shutdown has been blocked.

At the moment, processes that are not running in the same session as `ShutdownPatcher.exe` (e.g. services), will not be able to notify you that a shutdown has been blocked. The shutdown should still be blocked, however.

Tip: You can check which processes are patched by using [Process Explorer](http://technet.microsoft.com/en-us/sysinternals/bb896653.aspx). Use the search dialog and search for `patch.dll` and `patch_x64.dll`.


# Download #

To download ShutdownPatcher, go to the download section.


# PatchApps #

This feature was previously included with ShutdownGuard and was called `PatchApps`. I decided to separate it to a program of its own since it is quite experimental.
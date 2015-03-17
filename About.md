# Introduction #

I can't remember how I got the idea for ShutdownGuard. It was many years ago when I was learning to code in Delphi. Anyway, I never really released that version back then.

Now that I know C, I was looking through some of my old programs, searching for one that I could recode in C. I picked ShutdownGuard. The result is this port which is a much improved version of my old Delphi version. You can still download the old Delphi program and its source code from the download section.

Note that programs are still able to shutdown Windows if they explicitly force it. ShutdownGuard can **not** prevent all shutdowns.

## Windows XP ##

This will appear in Windows XP when a shutdown is blocked.

![http://shutdownguard.googlecode.com/svn/wiki/ShutdownGuard.png](http://shutdownguard.googlecode.com/svn/wiki/ShutdownGuard.png)

## Vista and Windows 7 ##

This will appear in Vista and Windows 7.

![http://shutdownguard.googlecode.com/svn/wiki/win7.png](http://shutdownguard.googlecode.com/svn/wiki/win7.png)


# Configuration #

There are a few features that you can configure in the configuration file called `ShutdownGuard.ini`. The simplest way to open it is through the tray menu.

## PreventMessage ##

This is simply a customized message that will appear when preventing a shutdown.

## Silent ##

If `Silent` is enabled and you have hidden the tray icon, ShutdownGuard won't notify you with the balloon tooltip when a shutdown is prevented.
You must hold the shift button when attempting shutdown to make the balloon tooltip show again.
Note that `Silent` does not work on Vista or Windows 7.

## HelpUrl ##

If `HelpUrl` is specified, a help button that will open the url will appear in the "_What do you want to do?_" dialog.
The url must start with _http://_ or _https://_.

## Language ##

This settings lets you change the language of ShutdownGuard. You can currently choose between English, Spanish, Galician, Nynorsk and Dutch.

If you want to translate ShutdownGuard, feel free to email me, but please only do this if you want to commit to keeping your translation up to date in the future.

## Update ##

ShutdownGuard will by default check if there is a newer version available when it is started. It will only notify you if it finds a new version, it will not download anything for you. If you do not like this behavior, you can disable it here. You can check for update manually in the tray menu, regardless of this setting.

Remember that you can monitor [the rss feed](http://shutdownguard.googlecode.com/svn/wiki/feed.xml) for new releases.


# UAC #

ShutdownGuard does not require administrator privileges. If you installed ShutdownGuard in `C:\Program Files\`, then you need administrator privileges to edit `ShutdownGuard.ini`. The easiest way to fix this is by right clicking on `ShutdownGuard.ini`, click _Properties_, then go to the _Security_ tab and give the _Users_ group _Full control_ to the file.

If you want ShutdownGuard to launch with administrator privileges on startup, then you have to configure the task scheduler to launch ShutdownGuard on log in. Read [this blog post](http://botsikas.blogspot.com/2010/05/autostart-application-that-requires-uac.html) for a tutorial. There is one caveat though, you need to configure **a delay of 30 seconds** before the task is started. Otherwise the tray icon won't be added properly and ShutdownGuard will not work. You must also disable the autostart option in the tray menu. Successfully configuring a task like this will allow ShutdownGuard to be launched on startup with administrator privileges without a UAC prompt.


# About the author #

ShutdownGuard was created and is maintained by Stefan Sundin. You can visit my website at [stefansundin.com](http://stefansundin.com/).
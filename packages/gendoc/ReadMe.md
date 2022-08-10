# Purr Data @version@ @osname@ (@build-version@)

Pd is a real-time, graphical programming language for media processing created by Miller S. Puckette. It provides an environment for audio analysis, synthesis, and processing, with a rich set of multimedia capabilities. You can get Pd for Linux, Windows, Mac OSX, and BSD.

Purr Data was created by Jonathan Wilkes as a portable version of Ico Bukvic's Pd-l2ork, which in turn was based on Hans-Christoph Steiner's popular Pd-extended, a distribution of Pd which included a lot of externals. Purr Data is a modern successor of Pd-extended which features many enhancements, especially in the user interface which was rewritten from scratch using modern web technologies. It is available for Linux, Mac, and Windows.

- If you're new to Purr Data, you may first want to read our little tutorial introduction "[Meet the Cat](https://agraef.github.io/purr-data-intro)".

- More information about Purr Data can be found on the [GitHub mirror](https://agraef.github.io/purr-data/) maintained by Albert Gräf.

- Jonathan Wilkes' source code repository is available on [GitLab](https://git.purrdata.net/jwilkes/purr-data); this is where you should go for submitting bug reports and pull requests.

- Information about Ico Bukvic's Pd-l2ork version (which has been updated in 2021 and is now based on an earlier Purr Data version) can be found on the [Pd-l2ork website](https://l2ork.music.vt.edu/main/make-your-own-l2ork/software/).

- General information about Pd is available at <https://puredata.info>.

@linux@
This is the @osname@ build of the Purr Data package.
@linux@
@darwin@
This is the @osname@ (Mac OSX) build of the Purr Data package.
@darwin@
@mingw@
This is the @osname@ (Windows) build of the Purr Data package.
@mingw@
Purr Data is available for a variety of different platforms, and you can also compile it from source if you have the necessary development tools. Please check the [wiki](https://github.com/agraef/purr-data/wiki/Installation) for detailed installation instructions.

## Installation

@linux@
The [JGU Packages](https://build.opensuse.org/project/show/home:aggraef:purr-data-jgu) are a complete collection of Purr Data packages for Arch Linux, Fedora, and recent Debian, Raspbian and Ubuntu releases. They are maintained by Albert Gräf from the Johannes Gutenberg University (JGU) and hosted at the OBS (Open Build Service) by openSUSE. Installation instructions can be found in the [wiki](https://github.com/agraef/purr-data/wiki/Installation#linux).
@linux@

@darwin@
The latest Mac packages are available as disk images on [GitHub](https://github.com/agraef/purr-data/releases). To install Purr Data, drag the Pd-l2ork.app bundle in the disk image into your /Applications folder, or to the shortcut in the disk image.

If you want to use PDP on Mac OS X, you may have to install X11. For details see [How do I install Pd on MacOS X?](https://puredata.info/docs/faq/macosx)
@darwin@

@mingw@
The latest Windows packages are available as Inno Setup installers on [GitHub](https://github.com/agraef/purr-data/releases). Double-click the installer and follow the instructions. If you have a prior Purr Data installation, the installer will prompt you to first remove it. If your existing installation is much older, it is always a good idea to do this, in order to prevent garbled installations which mix files and registry entries of both versions.

[ASIO4ALL](https://www.asio4all.com/) is a cost-free [ASIO](https://en.wikipedia.org/wiki/Audio_Stream_Input/Output) audio driver. If you have it installed then you can enable it in Purr Data by choosing the **ASIO** option in the **Audio** tab of the Preferences dialog. Using ASIO4ALL will often give you lower latency and generally better audio performance.
@mingw@

## Usage

@linux@
If you installed Purr Data from the JGU repositories, then the executable will be named `purr-data`, and your desktop menus should contain an entry for `Purr-Data`. Use the former to launch Purr Data from the shell, and the latter to run the program from your desktop environment as usual.
@linux@

@darwin@
After installation you will find `Pd-l2ork` in the Launchpad, which can be run by clicking on the icon as usual. Note that Purr Data is distributed as an application bundle on the Mac, but you can use the `open` command to launch it from the shell as follows: `open -a Pd-l2ork`
@darwin@

@mingw@
After installation you will find an entry for `Purr-Data` in the start menu, which can be run by clicking on the icon as usual. If you opted to create a desktop and/or quick-launch icon during installation, you should be able to find these on your desktop as well.

Purr Data can also be launched from the Windows command line. The executable is named `pd.com` and is located in the `%ProgramFiles%\Purr Data\bin` directory (`%ProgramFiles(x86)%\Purr Data\bin` if you installed the 32 bit version). For instance, using the command prompt, i.e., cmd.exe, you can launch the program as follows (the quotes are needed since the path contains spaces):

~~~
"%ProgramFiles%\Purr Data\bin\pd"
~~~
@mingw@

## Installing Externals, Objects, and Help files

If you would like to install other externals, objects, help files, etc., there are special folders that Purr Data uses, which are listed below. If the folder does not exist, you should create it. You can find out more details about this by reading this FAQ: [How do I install externals and help files?](https://puredata.info/docs/faq/how-do-i-install-externals-and-help-files)

@linux@
- Only for the current user account: ~/pd-l2ork-externals
- For all user accounts on the computer: /usr/local/lib/pd-l2ork-externals
@linux@

@darwin@
- Only for the current user account: ~/Library/Pd-l2ork
- For all user accounts on the computer: /Library/Pd-l2ork
@darwin@

@mingw@
- Only for the current user account (open in File Explorer): [%AppData%\\Pd-l2ork](file:///%AppData%)
- For all user accounts on the computer (open in File Explorer): [%CommonProgramFiles%\\Pd-l2ork](file:///%CommonProgramFiles%)
@mingw@

## Configuration

By default, most of the included libraries are loaded at startup. To change this, set your own preferences in the **Startup** tab of the Preferences dialog.

@linux@
On Linux, configuration data (user preferences, help index, completions) lives in ~/.purr-data. You can remove this directory (`rm -rf ~/.purr-data`) to reset Purr Data to its defaults. This will also reset the help index and completion data (they will be re-created automatically).
@linux@

@darwin@
On the Mac, the user preferences live in ~/Library/Preferences/org.puredata.pd-l2ork.plist. You can remove that file (`rm ~/Library/Preferences/org.puredata.pd-l2ork.plist`) to reset Purr Data to the default preferences.

Some auxiliary configuration data (help index, completions) can be found in ~/.purr-data. You can remove this directory (`rm -rf ~/.purr-data`) to reset the help index and completion data (they will be re-created automatically).
@darwin@

@mingw@
On Windows, the user preferences live in the Windows registry. You *can* delete the `[HKEY_LOCAL_MACHINE\SOFTWARE\Purr-Data]` key and all its subkeys, but that will give you a completely "vanilla" setup with *no* preloaded libraries at all. Some people may even like that, in order to build their own library configuration. But the easiest way to reset the registry to some sane defaults which preloads most included libraries, is to reinstall the program.

Some auxiliary configuration data (help index, completions) is stored in [%AppData%\\Purr-Data](file:///%AppData%/Purr-Data). You can delete those files to reset the help index and completion data (they will be re-created automatically).
@mingw@

## License

This package is released under the [GNU GPL](https://www.gnu.org/copyleft/gpl.html). The Pd core and some other included code is available under a [BSD license](https://github.com/pure-data/pure-data/blob/master/LICENSE.txt) from the Pd source code repository on [GitHub](https://github.com/pure-data/pure-data).

## Patented Algorithms

This package may contain software that is covered by patents in certain countries, like the U.S. and Germany. In order to use this software you must have the proper license. Known software packages that are covered by patents in some countries are [MP3](https://en.wikipedia.org/wiki/MP3) (a.k.a. MPEG-1 Layer 3), [MPEG-2](https://en.wikipedia.org/wiki/MPEG-2), [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4). (Note that many of the MPEG-related patents have now expired in most countries; please check the linked Wikipedia articles for details.)

Please consider trying to get rid of software patents in your country:
<https://www.nosoftwarepatents.com>

This document was created @date@.

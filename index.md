---
layout: default
---

Jonathan Wilkes' **Purr Data** a.k.a. **Pd-l2ork 2** is an improved version of Miller Puckette's **[Pd](http://puredata.info/)**. It is based on Ico Bukvic's **[Pd-l2ork](http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/)**, which in turn is a fork of Hans-Christoph Steiner's **[Pd-extended](http://puredata.info/downloads/pd-extended)**.

This repository is a mirror of Wilkes' original Gitlab repository available at <https://git.purrdata.net/jwilkes/purr-data>. We mainly use this as a one-stop shop to make it easy for you to get your hands on the latest [source](https://github.com/agraef/purr-data) and the available [releases](https://github.com/agraef/purr-data/releases), including pre-built packages for Linux, macOS and Windows.

Here's a quick shopping list:

- [**Downloads**](https://github.com/agraef/purr-data/releases): This is where you get ready-made packages for **Mac and Windows**. Please also make sure to check our [installation instructions](#installation) below.
- [**JGU packages**](#jgu-packages): Use these if you're on **Arch**, **Fedora**, or a recent **Debian/Ubuntu** version. We offer proper package repositories including all dependencies which aren't readily available in the official repositories of your Linux distribution. **[Download](https://software.opensuse.org/download/package?package=purr-data&project=home%3Aaggraef%3Apurr-data-jgu)**
- [**Sources**](https://github.com/agraef/purr-data): Github mirror of the sources, updated regularly.
- [**Development**](https://git.purrdata.net/jwilkes/purr-data): Wilkes' Gitlab repository. **This is where you should go for submitting bug reports and pull requests.**
- [**Mailing list**](http://disis.music.vt.edu/listinfo/l2ork-dev): The DISIS Pd-l2ork mailing list is the right place for getting help and discuss Purr Data, so please subscribe!
- [**"Meet the Cat" Tutorial**](https://agraef.github.io/purr-data-intro): A quick introduction to Purr Data, how it came about, and how to use it. (French translation by Joseph Gastelais: [Rencontrez le Chat](https://www.linuxrouen.fr/wp/programmation/rencontrez-le-chat-introduction-rapide-a-purr-data-vs-pd-l2ork-vs-pd-extended-vs-pure-data-28047/))

There's also a [video](https://www.youtube.com/watch?v=T1wo496Zx0s) up on YouTube of the presentation *Meet the Cat: Pd-L2Ork and its New Cross-Platform Version "Purr Data"* at the Linux Audio Conference 2017 (Jean MONNET University, Saint-Etienne).

Enjoy your stay!

## About Purr Data

Pd (Pure Data) is a graphical data-flow programming environment which is geared towards real-time interactive computer music and multimedia applications. It is a full-featured open-source alternative to its commercial sibling, Cycling74's [Max](https://cycling74.com/).

Purr Data serves the same purpose, but offers a new and much improved graphical user interface and includes many 3rd party plug-ins. Like Pd, it runs on Linux, macOS and Windows, and is open-source throughout.

![Purr Data running on macOS.](purr-data.png){:width="100%"}

Purr Data also offers some notable advancements over "classic" Pd-l2ork:

- cross platform compatibility
- modern GUI written in JavaScript (using [nw.js](https://nwjs.io/))
- improved SVG graphics

Purr Data continues to offer all of Pd-l2ork's GUI and usability improvements, a help browser giving access to help patches in PDDP (Pd Documentation Project) format, and a large collection of bundled 3rd party externals, while using the latest and greatest version of Pd's tried and proven real-time engine under the hood.

## The Name?

Purr Data is the official nickname of the Pd-l2ork 2.x branch. Quite obviously the name is a play on "Pure Data" on which "Purr Data" is ultimately based. It also raises positive connotations of soothing purring sounds, and makes for a nice logo. 😺

## Installation

The easiest way to get up and running on **Mac** and **Windows** is to use one of the available binary packages and installers available from the Github [releases page](https://github.com/agraef/purr-data/releases). Generally you can just double-click these packages and go through the usual (platform-dependent) installation process.

**Linux users:** On OBS we provide the **JGU packages** for a variety of Linux distributions, see [below](#jgu-packages). If your Linux distro is not among any of these then sorry, you'll just have to bite the bullet and build Purr Data [from source](#building-from-source). (It's not that hard any more once you got all the required dependencies installed, but it may take a little while.)

### JGU Packages

This is a collection of ready-made Linux packages for Arch Linux, Fedora, and recent Debian, Raspbian and Ubuntu releases, maintained by the Johannes Gutenberg University (JGU) at the [OBS](https://build.opensuse.org/project/show/home:aggraef:purr-data-jgu) (Open Build System). They offer the following advantages:

- Support for a wide range of different Linux systems, including the latest Debian/Ubuntu versions. We generally support the two most recent stable (or long term support) releases, as well as current (or rolling) releases.

- "Classic" Pd-l2ork and Purr Data can be installed alongside each other.

- The packages are available through proper package repositories (click **[here](https://software.opensuse.org/download/package?package=purr-data&project=home%3Aaggraef%3Apurr-data-jgu)**) and thus can be installed and updated automatically through your distribution's standard package manager. Required dependencies will also be installed automatically.

- The packages are thoroughly tested, but are updated more frequently (also between upstream releases) from the current git sources, if there are interesting new features or important bugfixes. Corresponding Mac and Windows packages for such pre-releases can then usually be found on the Github [releases page](https://github.com/agraef/purr-data/releases).

Please refer to the [Installation](https://github.com/agraef/purr-data/wiki/Installation#linux) wiki page for more information and detailed installation instructions.

### Building from Source

This is just a brief summary. Please check the [Installation](https://github.com/agraef/purr-data/wiki/Installation#installing-from-source) page in the wiki for more information.

Purr Data is a big package with many parts and many dependencies, building it from source can take a *long* time, even on modern hardware. So please be patient! :)

To build Purr Data from source, you'll have to clone the [git repository](https://github.com/agraef/purr-data) as follows:

    git clone https://github.com/agraef/purr-data

Or, if you prefer to build straight from Wilke's upstream repository:

    git clone https://git.purrdata.net/jwilkes/purr-data.git

Make sure that you have all the requisite dependencies installed ([Linux](https://github.com/agraef/purr-data/blob/master/README.md#linux), [Mac](https://github.com/agraef/purr-data/blob/master/README.md#osx-64-bit-using-homebrew), [Windows](https://github.com/agraef/purr-data/blob/master/README.md#windows-32-bit-using-msys2)), then run `make` in the toplevel source directory:

    make

On Debian/Ubuntu, Mac and Windows this should leave the ready-made package in the toplevel source directory. Install this as usual. On other Linux systems you can run the following command instead (this also works on Debian/Ubuntu, if you prefer this method or if you don't have the Debian packaging tools installed):

    sudo make install

You can uninstall the software again as follows:

    sudo make uninstall

Afterwards, to clean the source directory:

    make realclean

Note that this puts the source into pristine state again, like after a fresh checkout, so that you can run `make` from a clean slate again. Also, all build artifacts will be gone, and hence you won't be able to run `make install` or `make uninstall` any more. So you want to do this only *after* you've finished the installation process.

Good luck!

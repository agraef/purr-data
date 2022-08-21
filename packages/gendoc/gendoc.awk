#! /usr/bin/awk -f

# This is to be invoked with osname set to the OS identifier as returned by
# $(uname -s) on the target system, i.e., "Linux" on Linux, "Darwin" on the
# Mac, etc. Only the sections marked with the corresponding id (@linux@,
# @darwin@, and @mingw@) will be included. In addition, @version@ and
# @build-version@ will be replaced with the contents of the (string) variables
# version and build_version, respectively, if specified on the command line.

BEGIN {
    # Default for osname if not set.
    if (!osname) { "uname -s" | getline osname };
    # Grab the current date, so that we can substitute it.
    "date -u" | getline date
}

/^@mingw@$/ {
    keep = osname ~ /^MINGW/;
    init = !init;
    skip = init && !keep;
    skipped = !init && !keep;
    next
}
/^@!mingw@$/ {
    keep = osname ~ !/^MINGW/;
    init = !init;
    skip = init && !keep;
    skipped = !init && !keep;
    next
}
/^@linux@$/ {
    keep = osname == "Linux";
    init = !init;
    skip = init && !keep;
    skipped = !init && !keep;
    next
}
/^@!linux@$/ {
    keep = osname != "Linux";
    init = !init;
    skip = init && !keep;
    skipped = !init && !keep;
    next
}
/^@darwin@$/ {
    keep = osname == "Darwin";
    init = !init;
    skip = init && !keep;
    skipped = !init && !keep;
    next
}
/^@!darwin@$/ {
    keep = osname != "Darwin";
    init = !init;
    skip = init && !keep;
    skipped = !init && !keep;
    next
}

skip { next }
# Get rid of extra empty lines after a skipped section.
skipped && /^[[:space:]]*$/ { skipped = 0; next }

!skip {
    skipped = 0;
    line = $0;
    gsub(/@date@/, date, line)
    gsub(/@osname@/, osname, line)
    if (version) {
        gsub(/@version@/, version, line)
    }
    if (build_version) {
        gsub(/@build-version@/, build_version, line)
    }
    print(line)
}

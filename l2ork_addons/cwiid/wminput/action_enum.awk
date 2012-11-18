BEGIN {
	print "#include <stdlib.h>"
	print "#include <linux/input.h>"
	print "#include \"conf.h\""
	print
	print "struct lookup_enum action_enum[] = {"
}

/^#define/ {
	printf "#ifdef %s\n", $2
	printf "\t{\"%s\",\t%s},\n", $2, $2
	print "#endif"
}

END {
	print "\t{NULL, 0}"
	print "};"
}

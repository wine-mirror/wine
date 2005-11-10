BEGIN {
	print "/* Machine generated. Do not edit. */"
	print ""
	lines = 0
}
{ 
	split($0, array, FS)

	if (NF > 0 && length(array[1]) > 0) {
		lines++

		# save the first word (or '&' separated list of words) in the names array
		if (array[2] == "&") {
			if (array[4] == "&") {
				names[lines] = array[1] " " array[2] " " array[3] " " array[4] " " array[5]
				start =  6
			} else {
				names[lines] = array[1] " " array[2] " " array[3]
				start = 4 
			}
		} else {
			names[lines] = array[1] 
			start = 2
		}

		# create the WCHAR version of the name
		printf "static const WCHAR name%dW[] = { ", lines
		i = 1
		len = length(names[lines]) + 1
		while (i < len) {
			printf "'%s',", substr(names[lines],i,1)
			i++
		}
		print  "0 };"
	
		# create the CHAR version of the description
		printf "static const CHAR description%dA[] = \"", lines
		word = start 
		while (word < (NF + 1)) {
			printf "%s", array[word]
			if (word < NF )
				printf " "
			word++
		}
		print  "\";"
	
		# create the WCHAR version of the description
		printf "static const WCHAR description%dW[] = { ", lines
		word = start
		while (word < (NF + 1)) {
			i = 1
			len = length(array[word]) + 1
			while (i < len) {
				if (substr(array[word],i,1) == "'")
					printf "'\\'',"
				else
					printf "'%s',", substr(array[word],i,1)
				i++
			}
			if (word < NF )
				printf "' ',"
			word++
		}
		print  "0 };"
	}
}
END {
	print ""
	print "static const error_info info[] = {"

	i = 1 
	while ( i <= lines) { 
		split(names[i], words, FS)
		printf "    { %s, \"%s\", name%dW, description%dA, description%dW },\n", 
			words[1], names[i], i, i,i 
		i++
	}

	print "};"
}
